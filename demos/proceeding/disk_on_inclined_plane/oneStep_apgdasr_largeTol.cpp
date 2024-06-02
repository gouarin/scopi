#include <scopi/objects/types/plan.hpp>
#include <scopi/objects/types/sphere.hpp>
#include <scopi/property.hpp>
#include <scopi/solver.hpp>
#include <vector>
#include <xtensor/xmath.hpp>

#include <scopi/solvers/OptimProjectedGradient.hpp>
#include <scopi/solvers/gradient/apgd_asr.hpp>
#include <scopi/vap/vap_fpd.hpp>

int main()
{
    // Table 1: Disk placed on a plane without friction. Number of iterations for the APGD-ASR algorithm and tolerance 10^{-3}.
    plog::init(plog::info, "disk_on_inclined_plane_oneStep_apdgasr_largeTol.log");

    constexpr std::size_t dim = 2;
    double PI                 = xt::numeric_constants<double>::PI;

    double radius = 1.;
    double g      = 1.;
    double mass   = 1.;
    double h      = radius;
    auto prop     = scopi::property<dim>().mass(mass).moment_inertia(mass * radius * radius / 2.);

    double dt            = 0.05;
    std::size_t total_it = 1;
    double alpha         = PI / 6.;

    scopi::scopi_container<dim> particles;
    scopi::plan<dim> p(
        {
            {0., 0.}
    },
        PI / 2. - alpha);
    scopi::sphere<dim> s(
        {
            {h * std::sin(alpha), h * std::cos(alpha)}
    },
        radius);
    particles.push_back(p, scopi::property<dim>().deactivate());
    particles.push_back(s,
                        prop.force({
                            {0., -g}
    }));

    scopi::ScopiSolver<dim, scopi::OptimProjectedGradient<scopi::DryWithoutFriction, scopi::apgd_asr>, scopi::contact_kdtree, scopi::vap_fpd>
        solver(particles, dt);
    auto params               = solver.get_params();
    params.optim_params.tol_l = 1e-3;
    params.optim_params.rho   = 2.;

    solver.run(total_it);

    return 0;
}
