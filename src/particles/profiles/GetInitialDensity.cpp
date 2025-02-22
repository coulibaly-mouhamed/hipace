#include "GetInitialDensity.H"
#include <AMReX_ParmParse.H>

GetInitialDensity::GetInitialDensity (const std::string& name)
{
    amrex::ParmParse pp(name);
    std::string profile;
    pp.get("density", m_density);
    pp.get("profile", profile);

    if        (profile == "gaussian") {
        m_profile = BeamProfileType::Gaussian;
    } else if (profile == "flattop") {
        m_profile = BeamProfileType::Flattop;
    } else {
        amrex::Abort("Unknown beam profile!");
    }

    if (m_profile == BeamProfileType::Gaussian) {
        amrex::Array<amrex::Real, AMREX_SPACEDIM> loc_array;
        if (pp.query("position_mean", loc_array)) {
            for (int idim=0; idim < AMREX_SPACEDIM; ++idim) {
                m_position_mean[idim] = loc_array[idim];
            }
        }
        if (pp.query("position_std", loc_array)) {
            for (int idim=0; idim < AMREX_SPACEDIM; ++idim) {
                m_position_std[idim] = loc_array[idim];
            }
        }
    }
}
