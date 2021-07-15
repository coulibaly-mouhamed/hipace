#include "BeamParticleAdvance.H"
#include "particles/ExternalFields.H"
#include "FieldGather.H"
#include "utils/Constants.H"
#include "GetAndSetPosition.H"
#include "utils/HipaceProfilerWrapper.H"

void
AdvanceBeamParticlesSlice (BeamParticleContainer& beam, Fields& fields, amrex::Geometry const& gm,
                           int const lev, const int islice, const amrex::Box box, const int offset,
                           BeamBins& bins)
{
    HIPACE_PROFILE("AdvanceBeamParticlesSlice()");
    using namespace amrex::literals;

    // only finest MR level pushes the beam
    if (beam.m_finest_level != lev) return;

    // Extract properties associated with physical size of the box
    amrex::Real const * AMREX_RESTRICT dx = gm.CellSize();
    const PhysConst phys_const = get_phys_const();

    const bool do_z_push = beam.m_do_z_push;
    const int n_subcycles = beam.m_n_subcycles;
    const amrex::Real dt = Hipace::m_dt / n_subcycles;

    // Assumes '2' == 'z' == 'the long dimension'.
    int islice_local = islice - box.smallEnd(2);

    // Extract properties associated with the extent of the current box
    const int depos_order_xy = Hipace::m_depos_order_xy;
    amrex::Box tilebox = box;
    tilebox.grow({depos_order_xy, depos_order_xy, Hipace::m_depos_order_z});

    amrex::RealBox const grid_box{tilebox, gm.CellSize(), gm.ProbLo()};
    amrex::Real const * AMREX_RESTRICT xyzmin = grid_box.lo();
    amrex::Dim3 const lo = amrex::lbound(tilebox);

    // Extract the fields
    const amrex::MultiFab& S = fields.getSlices(lev, WhichSlice::This);
    const amrex::MultiFab exmby(S, amrex::make_alias, Comps[WhichSlice::This]["ExmBy"], 1);
    const amrex::MultiFab eypbx(S, amrex::make_alias, Comps[WhichSlice::This]["EypBx"], 1);
    const amrex::MultiFab ez(S, amrex::make_alias, Comps[WhichSlice::This]["Ez"], 1);
    const amrex::MultiFab bx(S, amrex::make_alias, Comps[WhichSlice::This]["Bx"], 1);
    const amrex::MultiFab by(S, amrex::make_alias, Comps[WhichSlice::This]["By"], 1);
    const amrex::MultiFab bz(S, amrex::make_alias, Comps[WhichSlice::This]["Bz"], 1);

    // Extract field array from FabArrays in MultiFabs.
    // (because there is currently no transverse parallelization, the index
    // we want in the slice multifab is always 0. Fix later.
    amrex::Array4<const amrex::Real> const& exmby_arr = exmby[0].array();
    amrex::Array4<const amrex::Real> const& eypbx_arr = eypbx[0].array();
    amrex::Array4<const amrex::Real> const& ez_arr = ez[0].array();
    amrex::Array4<const amrex::Real> const& bx_arr = bx[0].array();
    amrex::Array4<const amrex::Real> const& by_arr = by[0].array();
    amrex::Array4<const amrex::Real> const& bz_arr = bz[0].array();

    const amrex::GpuArray<amrex::Real, 3> dx_arr = {dx[0], dx[1], dx[2]};
    const amrex::GpuArray<amrex::Real, 3> xyzmin_arr = {xyzmin[0], xyzmin[1], xyzmin[2]};

    // Extract particle properties
    auto& soa = beam.GetStructOfArrays(); // For momenta and weights
    amrex::Real * const uxp = soa.GetRealData(BeamIdx::ux).data() + offset;
    amrex::Real * const uyp = soa.GetRealData(BeamIdx::uy).data() + offset;
    amrex::Real * const uzp = soa.GetRealData(BeamIdx::uz).data() + offset;

    const auto getPosition = GetParticlePosition<BeamParticleContainer>(beam, offset);
    const auto setPosition = SetParticlePosition<BeamParticleContainer>(beam, offset);
    const auto enforceBC = EnforceBC<BeamParticleContainer>(beam, lev, box, offset);

    const amrex::Real zmin = xyzmin[2];

    // Declare a DenseBins to pass it to doDepositionShapeN, although it will not be used.
    BeamBins::index_type*
        indices = nullptr;
    BeamBins::index_type const *
        offsets = nullptr;
    indices = bins.permutationPtr();
    offsets = bins.offsetsPtr();
    BeamBins::index_type const
        cell_start = offsets[islice_local], cell_stop = offsets[islice_local+1];
    // The particles that are in slice islice_local are
    // given by the indices[cell_start:cell_stop]

    int const num_particles = cell_stop-cell_start;

    const amrex::Real clightsq = 1.0_rt/(phys_const.c*phys_const.c);
    const amrex::Real charge_mass_ratio = beam.m_charge / beam.m_mass;
    const amrex::Real external_ExmBy_slope = Hipace::m_external_ExmBy_slope;
    const amrex::Real external_Ez_slope = Hipace::m_external_Ez_slope;
    const amrex::Real external_Ez_uniform = Hipace::m_external_Ez_uniform;

    amrex::ParallelFor(
        num_particles,
        [=] AMREX_GPU_DEVICE (long idx) {
            const int ip = indices[cell_start+idx];

            amrex::ParticleReal xp, yp, zp;
            int pid;

            for (int i = 0; i < n_subcycles; i++) {

                getPosition(ip, xp, yp, zp, pid);
                if (pid < 0) return;

                const amrex::ParticleReal gammap = sqrt(
                    1.0_rt + uxp[ip]*uxp[ip]*clightsq
                    + uyp[ip]*uyp[ip]*clightsq + uzp[ip]*uzp[ip]*clightsq);

                // first we do half a step in x,y
                // This is not required in z, which is pushed in one step later
                xp += dt * 0.5_rt * uxp[ip] / gammap;
                yp += dt * 0.5_rt * uyp[ip] / gammap;

                setPosition(ip, xp, yp, zp);
                if (enforceBC(ip, lev)) return;

                // define field at particle position reals
                amrex::ParticleReal ExmByp = 0._rt, EypBxp = 0._rt, Ezp = 0._rt;
                amrex::ParticleReal Bxp = 0._rt, Byp = 0._rt, Bzp = 0._rt;

                // field gather for a single particle
                doGatherShapeN(xp, yp, zmin,
                               ExmByp, EypBxp, Ezp, Bxp, Byp, Bzp,
                               exmby_arr, eypbx_arr, ez_arr, bx_arr, by_arr, bz_arr,
                               dx_arr, xyzmin_arr, lo, depos_order_xy, 0);

                ApplyExternalField(xp, yp, zp, ExmByp, EypBxp, Ezp,
                                   external_ExmBy_slope, external_Ez_slope, external_Ez_uniform);

                // use intermediate fields to calculate next (n+1) transverse momenta
                const amrex::ParticleReal ux_next = uxp[ip] + dt * charge_mass_ratio
                    * ( ExmByp + ( phys_const.c - uzp[ip] / gammap ) * Byp );
                const amrex::ParticleReal uy_next = uyp[ip] + dt * charge_mass_ratio
                    * ( EypBxp + ( uzp[ip] / gammap - phys_const.c ) * Bxp );

                // Now computing new longitudinal momentum
                const amrex::ParticleReal ux_intermediate = ( ux_next + uxp[ip] ) * 0.5_rt;
                const amrex::ParticleReal uy_intermediate = ( uy_next + uyp[ip] ) * 0.5_rt;
                const amrex::ParticleReal uz_intermediate = uzp[ip]
                    + dt * 0.5_rt * charge_mass_ratio * Ezp;

                const amrex::ParticleReal gamma_intermediate = sqrt(
                    1.0_rt + ux_intermediate*ux_intermediate*clightsq +
                    uy_intermediate*uy_intermediate*clightsq +
                    uz_intermediate*uz_intermediate*clightsq );

                const amrex::ParticleReal uz_next = uzp[ip] + dt * charge_mass_ratio
                    * ( Ezp + ( ux_intermediate * Byp - uy_intermediate * Bxp )
                        / gamma_intermediate );

                /* computing next gamma value */
                const amrex::ParticleReal gamma_next = sqrt( 1.0_rt + uz_next*uz_next*clightsq
                                                             + ux_next*ux_next*clightsq
                                                             + uy_next*uy_next*clightsq );

                /*
                 * computing positions and setting momenta for the next timestep
                 *(n+1)
                 * The longitudinal position is updated here as well, but in
                 * first-order (i.e. without the intermediary half-step) using
                 * a simple Galilean transformation
                 */
                xp += dt * 0.5_rt * ux_next  / gamma_next;
                yp += dt * 0.5_rt * uy_next  / gamma_next;
                if (do_z_push) zp += dt * ( uz_next  / gamma_next - phys_const.c );
                setPosition(ip, xp, yp, zp);
                if (enforceBC(ip, lev)) return;
                uxp[ip] = ux_next;
                uyp[ip] = uy_next;
                uzp[ip] = uz_next;
            } // end for loop over n_subcycles
        });
}
