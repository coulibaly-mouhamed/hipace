.. _parameters-source:

Input parameters
================

General parameters
------------------

* ``amr.n_cell`` (3 `integer`)
    Number of cells in x, y and z.

* ``amr.max_level`` (`integer`)
    Maximum level of mesh refinement. Mesh refinement is not yet implemented, therefore only
    level `0` is supported.

* ``hipace.patch_lo`` (3 `float`)
    Lower end of the refined grid in x, y and z.

* ``hipace.patch_hi`` (3 `float`)
    Upper end of the refined grid in x, y and z.

* ``amr.ref_ratio_vect`` (3 `int`)
    Refinement ratio. Last one must be 1.

* ``max_step`` (`integer`) optional (default `0`)
    Maximum number of time steps. `0` means that the 0th time step will be calculated, which are the
    fields of the initial beams.

* ``hipace.dt`` (`float`) optional (default `0.`)
    Time step to advance the particle beam.

* ``hipace.normalized_units`` (`bool`) optional (default `0`)
    Using normalized units in the simulation.

* ``hipace.verbose`` (`int`) optional (default `0`)
    Level of verbosity.

      * `verbose = 1`, prints only the time steps, which are computed.

      * `verbose = 2` additionally prints the number of iterations in the
        predictor-corrector loop, as well as the B-Field error at each slice.

      * `verbose = 3` also prints the number of particles, which violate the quasi-static
        approximation and were neglected at each slice. It prints the number of ionized particles,
        if ionization occurred. It also adds additional information if beams
        are read in from file.

* ``hipace.depos_order_xy`` (`int`) optional (default `2`)
    Transverse particle shape order. Currently, `0,1,2,3` are implemented.

* ``hipace.depos_order_z`` (`int`) optional (default `0`)
    Transverse particle shape order. Currently, only `0` is implemented.

* ``hipace.output_period`` (`integer`) optional (default `-1`)
    | Output period. No output is given for `hipace.output_period = -1`.
    | **Warning:** `hipace.output_period = 0` will make the simulation crash.

* ``hipace.beam_injection_cr`` (`integer`) optional (default `1`)
    | Using a temporary coarsed grid for beam particle injection for a fixed particle-per-cell beam.
      For very high-resolution simulations, where the number of grid points (`nx*ny*nz`)
      exceeds the maximum `int (~2e9)`, it enables beam particle injection, which would
      fail otherwise. As an example, a simulation with `(2048*2048*2048)` grid points
      requires
    | `hipace.beam_injection_cr = 8`.

* ``hipace.do_beam_jx_jy_deposition`` (`bool`) optional (default `1`)
    Using the default, the beam deposits all currents `Jx`, `Jy`, `Jz`. Using
    `hipace.do_beam_jx_jy_deposition = 0` disables the transverse current deposition of the beams.

* ``hipace.boxes_in_z`` (`int`) optional (default `1`)
    Number of boxes along the z-axis. In serial runs, the arrays for 3D IO can easily exceed the
    memory of a GPU. Using multiple boxes reduces the memory requirements by the same factor.
    This option is only available in serial runs, in parallel runs, please use more GPU to achieve
    the same effect.

* ``hipace.openpmd_backend`` (`string`) optional (default `h5`)
    OpenPMD backend. This can either be `h5, bp`, or `json`. The default is chosen by what is
    available. If both Adios2 and HDF5 are available, `h5` is used. Note that `json` is extremely
    slow and is not recommended for production runs.

Field solver parameters
-----------------------

Two different field solvers are available to calculate the transverse magnetic fields `Bx`
and `By`. An FFT-based predictor-corrector loop and an analytic integration. In the analytic
integration the longitudinal derivative of the transverse currents is calculated explicitly, which
results in a Helmholtz equation, which is solved with the AMReX multigrid solver.
Currently, the default is to use the predictor-corrector loop.
Modeling ion motion is not yet supported by the explicit solver

* ``hipace.bxby_solver`` (`string`) optional (default `predictor-corrector`)
    Which solver to use.
    Possible values: ``predictor-corrector`` and ``explicit``.

* ``hipace.use_small_dst`` (`bool`) optional (default `0` or `1`)
    Whether to use a large R2C or a small C2R fft in the dst of the Poisson solver.
    The small dst is quicker for simulations with :math:`\geq 511` transverse grid points.
    The default is set accordingly.

Predictor-corrector loop parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

* ``hipace.predcorr_B_error_tolerance`` (`float`) optional (default `4e-2`)
    The tolerance of the transverse B-field error. To enable a fixed number of iterations,
    `predcorr_B_error_tolerance` must be negative.

* ``hipace.predcorr_max_iterations`` (`int`) optional (default `30`)
    The maximum number of iterations in the predictor-corrector loop for single slice.

* ``hipace.predcorr_B_mixing_factor`` (`float`) optional (default `0.05`)
    The mixing factor between the currently calculated B-field and the B-field of the
    previous iteration (or initial guess, in case of the first iteration).
    A higher mixing factor leads to a faster convergence, but increases the chance of divergence.

.. note::
   In general, we recommend two different settings:

   First, a fixed B-field error tolerance. This ensures the same level of convergence at each grid
   point. To do so, use e.g. the default settings of `hipace.predcorr_B_error_tolerance = 4e-2`,
   `hipace.predcorr_max_iterations = 30`, `hipace.predcorr_B_mixing_factor = 0.05`.
   This should almost always give reasonable results.

   Second, a fixed (low) number of iterations. This is usually much faster than the fixed B-field
   error, but can loose significant accuracy in special physical simulation settings. For most
   settings (e.g. a standard PWFA simulation the blowout regime at a reasonable resolution) it
   reproduces the same results as the fixed B-field error tolerance setting. It works very well at
   high longitudinal resolution.
   A good setting for the fixed number of iterations is usually given by
   `hipace.predcorr_B_error_tolerance = -1.`, `hipace.predcorr_max_iterations = 1`,
   `hipace.predcorr_B_mixing_factor = 0.15`. The B-field error tolerance must be negative.

Explicit solver parameters
^^^^^^^^^^^^^^^^^^^^^^^^^^

* ``hipace.MG_tolerance_rel`` (`float`) optional (default `1e-4`)
    Relative error tolerance of the AMReX multigrid solver.

* ``hipace.MG_tolerance_abs`` (`float`) optional (default `0.`)
    Absolute error tolerance of the AMReX multigrid solver.

Plasma parameters
-----------------

For the plasma parameters, first the names of the plasmas need to be specified. Afterwards, the
plasma parameters for each plasma are specified via `<plasma name>.plasma_property = ...`

* ``plasmas.names`` (`string`)
    The names of the plasmas, separated by a space.
    To run without plasma, choose the name `no_plasma`.

* ``<plasma name>.density`` (`float`) optional (default `0.`)
    The plasma density.

* ``<plasma name>.ppc`` (2 `integer`) optional (default `0 0`)
    The number of plasma particles per cell in x and y.
    Since in a quasi-static code, there is only a 2D plasma slice evolving along the longitudinal
    coordinate, there is no need to specify a number of particles per cell in z.

* ``<plasma name>.level`` (`integer`) optional (default `0`)
    Level of mesh refinement to which the plasma is assigned.

* ``<plasma name>.radius`` (`float`) optional (default `infinity`)
    Radius of the plasma. Set a value to run simulations in a plasma column.

* ``<plasma name>.hollow_core_radius`` (`float`) optional (default `0.`)
    Inner radius of a hollow core plasma. The hollow core radius must be smaller than the plasma
    radius itself.

* ``<plasma name>.parabolic_curvature`` (`float`) optional (default `0.`)
    Curvature of a parabolic plasma profile. The plasma density is set to
    :math:`\mathrm{plasma.density} * (1 + \mathrm{plasma.parabolic\_curvature}*r^2)`.

* ``<plasma name>.max_qsa_weighting_factor`` (`float`) optional (default `35.`)
    The maximum allowed weighting factor :math:`\gamma /(\psi+1)` before particles are considered
    as violating the quasi-static approximation and are removed from the simulation.

* ``<plasma name>.mass`` (`float`) optional (default `0.`)
    The mass of plasma particle in SI units. Use `plasma_name.mass_Da` for Dalton.
    Can also be set with `plasma_name.element`. Must be `>0`.

* ``<plasma name>.mass_Da`` (`float`) optional (default `0.`)
    The mass of plasma particle in Dalton. Use `plasma_name.mass` for SI units.
    Can also be set with `plasma_name.element`. Must be `>0`.

* ``<plasma name>.charge`` (`float`) optional (default `0.`)
    The charge of a plasma particle. Can also be set with `plasma_name.element`.
    The charge gets multiplied by the current ionization level.

* ``<plasma name>.element`` (`string`) optional (default "")
    The Physical Element of the plasma. Sets charge, mass and, if available,
    the specific Ionization Energy of each state.
    Options are: `electron`, `positron`, `H`, `D`, `T`, `He`, `Li`, `Be`, `B`, ….

* ``<plasma name>.can_ionize`` (`bool`) optional (default `0`)
    Whether this plasma can ionize. Can also be set to 1 by specifying
    `plasma_name.ionization_product`.

* ``<plasma name>.initial_ion_level`` (`int`) optional (default `-1`)
    The initial Ionization state of the plasma. `0` for neutral gasses.
    If set, the Plasma charge gets multiplied by this number.

* ``<plasma name>.ionization_product`` (`string`) optional (default "")
    The `plasma_name` of the plasma that contains the new electrons that are produced
    when this plasma gets ionized. Only needed if this plasma is ionizable.

Beam parameters
---------------

For the beam parameters, first the names of the beams need to be specified. Afterwards, the beam
parameters for each beam are specified via `<beam name>.beam_property = ...`

* ``beams.names`` (`string`)
    The names of the particle beams, separated by a space.
    To run without beams, choose the name `no_beam`.

* ``<beam name>.injection_type`` (`string`)
    The injection type for the particle beam. Currently available are `fixed_ppc`, `fixed_weight`,
    and `from_file`. `fixed_ppc` generates a beam with a fixed number of particles per cell and
    varying weights. It can be either a Gaussian or a flattop beam. `fixed_weight` generates a
    Gaussian beam with a fixed number of particles with a constant weight.
    `from_file` reads a beam from openPMD files.

* ``<beam name>.random_ppc`` (list of 3 `int`) optional (default `0 0 0`)
    Whether to randomize the position of particles in each cell, per dimension.
    Only used if ``<beam name>.injection_type = fixed_ppc``.

* ``<beam name>.profile`` (`string`)
    Beam profile.
    When ``<beam name>.injection_type == fixed_ppc``, possible options are ``flattop`` (flat-top radially and longitudinally) or ``gaussian`` (Gaussian in all directions).
    When ``<beam name>.injection_type == fixed_weight``, possible options are ``can`` (uniform longitudinally, Gaussian transversally) and ``gaussian`` (Gaussian in all directions).

* ``<beam name>.n_subcycles`` (`int`) optional (default `1`)
    Number of sub-cycles performed in the beam particle pusher. The particles will be pushed
    `n_subcycles` times with a time step of `dt/n_subcycles`. This can be used to improve accuracy
    in highly non-linear focusing fields.

Option: ``fixed_weight``
^^^^^^^^^^^^^^^^^^^^^^^^

* ``<beam name>.position_mean`` (3 `float`)
    The mean position of the beam in `x, y, z`, separated by a space.

* ``<beam name>.position_std`` (3 `float`)
    The rms size of the of the beam in `x, y, z`, separated by a space.

* ``<beam name>.num_particles`` (`int`)
    Number of constant weight particles to generate the beam.

* ``<beam name>.total_charge`` (`float`)
    Total charge of the beam. Note: Either `total_charge` or `density` must be specified.

* ``<beam name>.density`` (`float`)
    Peak density of the beam. Note: Either `total_charge` or `density` must be specified.

* ``<beam name>.dx_per_dzeta`` (`float`)  optional (default `0.`)
    Tilt of the beam in the x direction. The tilt is introduced with respect to the center of the
    beam.

* ``<beam name>.dy_per_dzeta`` (`float`)  optional (default `0.`)
    Tilt of the beam in the y direction. The tilt is introduced with respect to the center of the
    beam.

* ``<beam name>.duz_per_uz0_dzeta`` (`float`) optional (default `0.`)
    Relative correlated energy spread per :math:`\zeta`.
    Thereby, `duz_per_uz0_dzeta *` :math:`\zeta` `* uz_mean` is added to `uz` of the each particle.
    :math:`\zeta` is hereby the particle position relative to the mean
    longitudinal position of the beam.

* ``<beam name>.do_symmetrize`` (`bool`) optional (default `0`)
    Symmetrizes the beam in the transverse phase space. For each particle with (`x`, `y`, `ux`,
    `uy`), three further particles are generated with (`-x`, `y`, `-ux`, `uy`), (`x`, `-y`, `ux`,
    `-uy`), and (`-x`, `-y`, `-ux`, `-uy`). The total number of particles will still be
    `beam_name.num_particles`, therefore this option requires that the beam particle number must be
    divisible by 4.

* ``<beam name>.do_z_push`` (`bool`) optional (default `1`)
    Whether the beam particles are pushed along the z-axis. The momentum is still fully updated.
    Note: using `do_z_push = 0` results in unphysical behavior.

Option: ``from_file``
^^^^^^^^^^^^^^^^^^^^^

* ``<beam name>.input_file`` (`string`)
    Name of the input file. **Note:** Reading in files with digits in their names (e.g.
    `openpmd_002135.h5`) can be problematic, it is advised to read them via `openpmd_%T.h5` and then
    specify the iteration via `beam_name.iteration = 2135`.

* ``<beam name>.iteration`` (`integer`) optional (default `0`)
    Iteration of the openPMD file to be read in. If the openPMD file contains multiple iterations,
    or multiple openPMD files are read in, the iteration can be specified. **Note:** The physical
    time of the simulation is set to the time of the given iteration (if available).

* ``<beam name>.openPMD_species_name`` (`string`) optional (default `<beam name>`)
    Name of the beam to be read in. If an openPMD file contains multiple beams, the name of the beam
    needs to be specified.

* ``beams.all_from_file`` (`string`)
    Name of the input file for all beams. This macro then passes it down to all individual beams
    without a specified `injection_type`. Additionally the input parameters `beams.iteration`,
    `beams.plasma_density` and `beams.file_coordinates_xyz` are passed down if applicable.

Diagnostic parameters
---------------------


* ``diagnostic.diag_type`` (`string`)
    Type of field output. Available options are `xyz`, `xz`, `yz`. `xyz` generates a 3D field
    output. Note that this can cause memory problems in particular on GPUs as the full 3D arrays
    need to be allocated. `xz` and `yz` generate 2D field outputs at the center of the y-axis and
    x-axis, respectively. In case of an even number of grid points, the value will be averaged
    between the two inner grid points.

* ``diagnostic.field_data`` (`string`) optional (default `all`)
    Names of the fields written to file, separated by a space. The field names need to be `all`,
    `none` or a subset of `ExmBy EypBx Ez Bx By Bz jx jy jz jx_beam jy_beam jz_beam rho Psi`.
    **Note:** The option `none` only suppressed the output of the field data. To suppress any
    output, please use `hipace.output_period = -1`.

* ``diagnostic.beam_data`` (`string`) optional (default `all`)
    Names of the beams written to file, separated by a space. The beam names need to be `all`,
    `none` or a subset of `beams.names`.
    **Note:** The option `none` only suppressed the output of the beam data. To suppress any
    output, please use `hipace.output_period = -1`.
