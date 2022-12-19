#ifndef PARTICLES_H
#define PARTICLES_H

#include <algorithm>
#include <random>
#include <string>
#include <quadgrid_cpp.h>
#include <map>
#include <iostream>

struct
particles_t {

  using idx_t = quadgrid_t<std::vector<double>>::idx_t;
  std::vector<double> x;
  std::vector<double> y;

  std::map<std::string, std::vector<idx_t>> iprops;
  std::map<std::string, std::vector<double>> dprops;

  std::vector<double> M;
  std::map<idx_t, std::vector<idx_t>> grd_to_ptcl;
  const quadgrid_t<std::vector<double>>& grid;

  particles_t (idx_t n, const std::vector<std::string>& ipropnames,
	       const std::vector<std::string>& dpropnames,
	       const quadgrid_t<std::vector<double>>& grid_)
    : x(n, 0.0), y(n, 0.0), grid(grid_) {

    for (idx_t ii = 0; ii < ipropnames.size (); ++ii) {
      iprops[ipropnames[ii]].assign (n, 0);
    }

    for (idx_t ii = 0; ii < dpropnames.size (); ++ii) {
      dprops[dpropnames[ii]].assign (n, 0.0);
    }

    M = std::vector<double> (grid.num_global_nodes (), 0.0);
    build_mass ();

    random_particle_positions (n);

    init_particle_mesh ();
  };

  void
  init_particle_mesh () {

    for (auto & igrd : grd_to_ptcl)
      std::vector<idx_t>{}. swap (igrd.second);

    for (auto ii = 0; ii < x.size (); ++ii) {
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
    std::generate (x.begin (), x.end (),
		   [&] () { return dis (gen) * grid.num_cols () * grid.hx (); });
    std::generate (y.begin (), y.end (),
		   [&] () { return dis (gen) * grid.num_rows () * grid.hy (); });
  };

  void
  build_mass () {
    M.assign (M.size (), 0.0);
    for (auto icell = grid.begin_cell_sweep ();
	 icell != grid.end_cell_sweep (); ++icell) {
      for (auto inode = 0;
	   inode < quadgrid_t<std::vector<double>>::cell_t::nodes_per_cell;
	   ++inode) {
	M[icell->gt (inode)] += (grid.hx () / 2.) * (grid.hy () / 2.);
      }
    }
  };


  const std::string &
  getkey(std::map<std::string, std::vector<double>> const &varnames,
	 int ivar) const {
    return std::next (varnames.begin (), ivar)->first;
  };


  const std::string &
  getkey(std::vector<std::string> const &varnames,
	 std::size_t ivar) const {
    return varnames[ivar];
  };

  void
  p2g (std::map<std::string, std::vector<double>> & vars,
       bool apply_mass = false) const {
    p2g (vars, vars, vars, apply_mass);
  }

  template<typename GT, typename PT>
  void
  p2g (std::map<std::string, std::vector<double>> & vars,       
       PT const & pvarnames,
       GT const & gvarnames,
       bool apply_mass = false) const {
    double N = 0.0, xx = 0.0, yy = 0.0;
    idx_t idx = 0;

    for (auto icell = grid.begin_cell_sweep ();
	 icell != grid.end_cell_sweep (); ++icell) {

      if (grd_to_ptcl.count (icell->get_global_cell_idx ()) > 0)
	for (idx_t ii = 0;
	     ii < grd_to_ptcl.at (icell->get_global_cell_idx ()).size ();
	     ++ii) {
	  idx = grd_to_ptcl.at (icell->get_global_cell_idx ())[ii];
	  xx = x[idx];
	  yy = y[idx];

	  for (idx_t inode = 0; inode < 4; ++inode) {
	    N = icell->shp(xx, yy, inode);
	    for (std::size_t ivar = 0; ivar < gvarnames.size (); ++ivar) {	    
	      vars[getkey(gvarnames, ivar)][icell->gt(inode)]  +=
		N * dprops.at (getkey(pvarnames, ivar))[idx];
	    }
	  }
	}
    }

    if (apply_mass)
      for (std::size_t ivar = 0; ivar < gvarnames.size (); ++ivar)
	for (idx_t ii = 0; ii < M.size (); ++ii) {
	  vars[getkey(gvarnames, ivar)][ii]  /= M[ii];
	}
  };
  
/*  void
  p2gdx(std::map<std::string, std::vector<double>>& vars, bool apply_mass = false) const {
    double N = 0.0, xx = 0.0, yy = 0.0, Nx = 0.0, Ny = 0.0;
    idx_t idx = 0;

    for (auto icell = grid.begin_cell_sweep(); icell != grid.end_cell_sweep(); ++icell) {
      for (idx_t ii = 0; ii<grd_to_ptcl.at(icell->get_global_cell_idx()).size(); ++ii) {
	idx = grd_to_ptcl.at(icell->get_global_cell_idx())[ii];
	xx = x[idx];
	yy = y[idx];

	for (idx_t inode=0; inode<4; ++inode) {

	  N = icell->shp(xx,yy,inode);
	  Nx = icell->shg(xx,yy,0,inode);
	  Ny = icell->shg(xx,yy,1,inode);
	  for (auto &ivar : vars)
	    vars[ivar.first][icell->t(inode)] += Nx*dprops.at(ivar.first)[ii];
	  for (auto &ivar : vars)
	    vars[ivar.first][icell->t(inode)] += Ny*dprops.at(ivar.first)[ii];
	  

	}

      }

    }

    if (apply_mass)
      for (auto &ivar : vars)
	for (idx_t ii = 0; ii<M.size();+ii) {

	  vars[ivar.first][ii] /= M[ii];

	}

  }; */

  void
  g2p (const std::map<std::string, std::vector<double>>& vars,
       bool apply_mass) {
    g2p (vars, vars, vars, apply_mass);
  }

  template<typename GT, typename PT>
  void
  g2p (const std::map<std::string, std::vector<double>>& vars,
       GT const & gvarnames,
       PT const & pvarnames,
       bool apply_mass) {
 
    double N = 0.0, xx = 0.0, yy = 0.0;
    idx_t idx = 0;

    for (auto icell = grid.begin_cell_sweep ();
         icell != grid.end_cell_sweep (); ++icell) {
      
      if (grd_to_ptcl.count (icell->get_global_cell_idx ()) > 0)
        for (idx_t ii = 0;
             ii < grd_to_ptcl.at (icell->get_global_cell_idx ()).size ();
             ++ii) {

	  idx = grd_to_ptcl.at(icell->get_global_cell_idx ())[ii];
          xx = x[idx];
          yy = y[idx];

          for (idx_t inode = 0; inode < 4; ++inode) {
            N = apply_mass ?
	      icell->shp(xx, yy, inode) * M[icell->gt(inode)] :
	      icell->shp(xx, yy, inode);
            for (std::size_t ivar = 0; ivar < gvarnames.size (); ++ivar)
              dprops.at (getkey (pvarnames, ivar))[idx] += N * vars.at (getkey (gvarnames, ivar))[icell->gt(inode)];
          }
        }

    }

  };

};

#endif /* PARTICLES_H */
