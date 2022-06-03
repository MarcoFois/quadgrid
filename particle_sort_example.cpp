#include <random>
#include <quadgrid_cpp.h>

using idx_t = quadgrid_t<std::vector<double>>::idx_t;


struct particles_t {

  std::vector<idx_t> label;
  std::vector<double> x;
  std::vector<double> y;
  std::vector<double> mass;
  std::vector<double> xvelocity;
  std::vector<double> yvelocity;
  std::vector<double> energy;
  std::vector<double> M;
  std::map<idx_t, std::vector<idx_t>> grd_to_ptcl;
  const quadgrid_t<std::vector<double>>& grid;
  
  particles_t (idx_t n, const quadgrid_t<std::vector<double>>& grid_)
    : label(n,0), x(n,0.0), y(n,0.0), mass(n,1.0),
      xvelocity (n,1.0), yvelocity (n,1.0),
      energy (n,1.0), grid(grid_) {

    M = std::vector<double> (grid.num_global_nodes (), 0.0);
    build_mass ();
        
    random_particle_positions (n);
    
    idx_t ilab = 0;
    std::iota (label.begin (), label.end (), ilab);
    
    init_particle_mesh ();
  };

  void
  init_particle_mesh () {
    for (auto ii = 0; ii - x.size (); ++ii) {
      idx_t c = static_cast<idx_t> (std::floor (x[ii] / grid.hx ()));
      idx_t r = static_cast<idx_t> (std::floor (y[ii] / grid.hy ()));
      
      grd_to_ptcl[grid.sub2gind (r, c)].push_back (ii);
    }
  };
  
  void
  random_particle_positions (idx_t n) {
    std::random_device rd;  
    std::mt19937 gen (rd ()); 
    std::uniform_real_distribution<> dis (0.0, 1.0);
    std::generate (x.begin (), x.end (), [&] () { return dis (gen) * grid.num_cols () * grid.hx (); });
    std::generate (y.begin (), y.end (), [&] () { return dis (gen) * grid.num_rows () * grid.hy (); });
  };
  
  void
  build_mass () {
    M.assign (M.size (), 0.0);
    for (auto icell = grid.begin_cell_sweep ();
         icell != grid.end_cell_sweep (); ++icell) {      
      for (auto inode = 0; inode < quadgrid_t<std::vector<double>>::cell_t::nodes_per_cell; ++inode) {
        M[icell->gt (inode)] += (grid.hx () / 2.) * (grid.hy () / 2.);          
      }
    }
  };
  
  void
  p2g (std::vector<double>& gm, std::vector<double>& gvx, std::vector<double>& gvy, std::vector<double>& ge) const {
    double N = 0.0, xx = 0.0, yy = 0.0, mm = 0.0, vx = 0.0, vy = 0.0, ee = 0.0;
    idx_t idx = 0;
    for (auto icell = grid.begin_cell_sweep ();
         icell != grid.end_cell_sweep (); ++icell) {
      if (grd_to_ptcl.count (icell->get_global_cell_idx ()) > 0)
        for (idx_t ii = 0; ii < grd_to_ptcl.at (icell->get_global_cell_idx ()).size (); ++ii) {
          idx = grd_to_ptcl.at(icell->get_global_cell_idx ())[ii];
          xx = x[idx];
          yy = y[idx];
          mm = mass[idx];
          vx = xvelocity[idx];
          vy = yvelocity[idx];
          ee = energy[idx];
          for (idx_t inode = 0; inode < 4; ++inode) {
            N = icell->shp(xx, yy, inode);
            gm[icell->t(inode)]  += N * mm;
            gvx[icell->t(inode)] += N * vx;
            gvy[icell->t(inode)] += N * vy;
            ge[icell->t(inode)]  += N * ee;
          }                
        }
    }

    for (idx_t ii = 0; ii < gm.size (); ++ii) {
      gm[ii]  /= M[ii];
      gvx[ii] /= M[ii];
      gvy[ii] /= M[ii];
      ge[ii]  /= M[ii];
    }
  };

  void
  g2p (const std::vector<double>& gm, const std::vector<double>& gvx,
       const std::vector<double>& gvy, const std::vector<double>& ge) const {
    // TO DO : Interpolazione dalla griglia alle particelle 
  };

};

int
main (int argc, char *argv[]) {

  quadgrid_t<std::vector<double>> grid;
  grid.set_sizes (20, 20, 1./20., 1./20.);

  constexpr idx_t num_particles = 1000;
  particles_t ptcls (num_particles, grid);

  /*
  //
  // This will produce very verbose output
  //
  for (auto icell = grid.begin_cell_sweep ();
       icell != grid.end_cell_sweep (); ++icell) {

    std::cout << "cell " << icell->get_global_cell_idx ()
              << " xlims [" << icell->p (0, 0) << ", "
              << icell->p (0, 3) << "]" 
              << " ylims [" << icell->p (1, 0) << ", "
              << icell->p (1, 3) << "]" << std::endl;

    for (auto ii = 0; ii < ptcls.grd_to_ptcl.at (icell->get_global_cell_idx ()).size (); ++ii) {
      std::cout << "\tparticle " << ptcls.grd_to_ptcl.at(icell->get_global_cell_idx ())[ii]
                << ": (" << ptcls.x[ptcls.grd_to_ptcl.at(icell->get_global_cell_idx ())[ii]]
                << ", " << ptcls.y[ptcls.grd_to_ptcl.at(icell->get_global_cell_idx ())[ii]]
                << ")" << std::endl;
    }

  }
  */
  
  std::vector<double> rho (grid.num_global_nodes (), 0.0);
  std::vector<double> px (grid.num_global_nodes (), 0.0);
  std::vector<double> py (grid.num_global_nodes (), 0.0);
  std::vector<double> ie (grid.num_global_nodes (), 0.0);

  ptcls.p2g (rho, px, py, ie);
    
  for (auto ii : rho)
    std::cout << ii << std::endl;
  
  return 0;
};

