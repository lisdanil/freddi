#include <boost/program_options.hpp>
#include <cmath>
#include <fstream>
#include <gsl/gsl_const_cgsm.h>
#include <iostream>
#include <sstream>
#include <string>

#include "nonlinear_diffusion.hpp"
#include "spectrum.hpp"


namespace po = boost::program_options;
using namespace std;
using namespace std::placeholders;


double r_out_func(const double Mx, const double Mopt, const double P){
	const double semiAxis = cbrt( GSL_CONST_CGSM_GRAVITATIONAL_CONSTANT * ( Mx + Mopt ) * P * P / ( 4. * M_PI*M_PI ) );	
	const double q = cbrt(Mx / Mopt);
	const double roche = semiAxis * 0.49 * q*q / ( 0.6 * q*q + log(1.+q) ); // Volume radius of Roche lobe, approximation from Eggleton, P. P. 1983, ApJ, 268, 368
	return 0.8 * roche;
}

double r_in_func(double Mx, double A){ // From «Black Hole Accretion Disks», A.44 (p. 530)
			double Z1 = 1. + cbrt( (1.-A*A) ) * ( cbrt( (1.+A) ) + cbrt( (1.-A) ) );
			double Z2 = sqrt( 3.*A*A + Z1*Z1 );
			return GSL_CONST_CGSM_GRAVITATIONAL_CONSTANT * Mx / (GSL_CONST_CGSM_SPEED_OF_LIGHT * GSL_CONST_CGSM_SPEED_OF_LIGHT) * ( 3. + Z2 - sqrt( (3.-Z1) * (3.+Z1+2.*Z2) ) );
};


int main(int ac, char *av[]){
	const double DAY = 86400.;
	const double Angstrem = 1e-8;
	const double keV = 1000. * GSL_CONST_CGSM_ELECTRON_VOLT / GSL_CONST_CGSM_PLANCKS_CONSTANT_H;
	const double solar_radius = 6.955e10;
	const double kpc = 1000. * GSL_CONST_CGSM_PARSEC;

	const double lambdaB = 4400. * Angstrem;
	const double irr0B = 6.61e-9 / Angstrem;
    // Calculated for BUXTON & BAILYN 2004
	const double lambdaV = 5450. * Angstrem;
	const double irr0V = 2.90e-9 / Angstrem;

	double alpha = 0.55;
	double fc = 1.7;
    double kerr = 0.;
    double eta = 1./12.;
	double Mx = 7.5 * GSL_CONST_CGSM_SOLAR_MASS;
	double Mopt = 0.8 * GSL_CONST_CGSM_SOLAR_MASS;
	double P = 0.433 * DAY;
	double cosi = cos(20.7 / 180 * M_PI);
	double Distance = 10. * kpc;
    double r_in = r_in_func( Mx, kerr );
	double r_out = r_out_func( Mx, Mopt, P );
    double T_min_hot_disk = 8000;
    double k_irr = 0.05; //0.05; // (dlog H / dlog r - 1)
	double nu_min = 1.2 * keV;
	double nu_max = 37.2 * keV;
	int Nx = 1000;
    string grid_scale = "log";
	double Time = 100. * DAY;
	double tau = 0.25 * DAY;
	double eps = 1e-6;
    string bound_cond_type = "Teff";
	double F0_gauss = 1e37;
	double sigma_for_F_gauss = 5.;
	double r_gauss_cut_to_r_out = 0.01;
    double power_order = 6.;
    double kMdot_out = 2.;
	string output_dir = "data";
	bool output_fulldata = false;
    string initial_cond_shape = "power";

    double Mdot_in;
    double Mdot_out = 0.;

	{
		po::options_description desc("Allowed options");
		desc.add_options()
			( "help,h", "Produce help message" )
			( "fulldata,f", "Output files with radial structure for every computed time step. Default is output only sum.dat with integrated parameters for every time step" )
			( "alpha,a", po::value<double>(&alpha)->default_value(alpha), "Alpha parameter" )
            ( "kerr,A", po::value<double>(&kerr)->default_value(kerr), "Kerr parameter of the black hole" )
			( "dilution,D", po::value<double>(&fc)->default_value(fc), "Dilution parameter"  )
			( "Mopt,m",	po::value<double>()->default_value(Mopt/GSL_CONST_CGSM_SOLAR_MASS), "Mass of optical star, solar masses" )
			( "Mx,M", po::value<double>()->default_value(Mx/GSL_CONST_CGSM_SOLAR_MASS), "Mass of central object, solar masses" )
			( "period,P", po::value<double>()->default_value(P/DAY), "Orbital period of binary system, days" )
			( "rout,R", po::value<double>()->default_value(r_out/solar_radius), "Outer radius of the disk, solar radii. If it isn't setted than it will be calculated using Mx, Mopt and period" )
            ( "distance,r", po::value<double>()->default_value(Distance/kpc), "Distance to the system, kpc" )
			( "numin,u", po::value<double>()->default_value(nu_min/keV), "Lower bound of X-ray band, keV" )
			( "numax,U", po::value<double>()->default_value(nu_max/keV), "Upper bound of X-ray band, keV" )
			( "Nx,N",	po::value<int>(&Nx)->default_value(Nx), "Size of calculation grid" )
            ( "gridscale,g", po::value<string>(&grid_scale)->default_value(grid_scale), "Type of grid: log or linear" )
			( "tau,t",	po::value<double>()->default_value(tau/DAY), "Time step, days" )
			( "time,T", po::value<double>()->default_value(Time/DAY), "Computation time, days" )
            // ( "boundSigma,S", "Use 4*Sigma_min from Menou et al. 1999 as a boundary for a 'hot' disk" )
            // ( "boundMdot,b", "Use -2 * Mdot_in from  Menou et al. 1999 as a boundary for a 'hot' disk" )
            ( "boundcond,B", po::value<string>(&bound_cond_type)->default_value(bound_cond_type), "Boundary movement condition, should be one of: Teff, fourSigmaCrit, MdotOut" )
            ( "Thot,H", po::value<double>(&T_min_hot_disk)->default_value(T_min_hot_disk), "Minimum photosphere temperature of the outer edge of the hot disk, degrees Kelvin. This option works only with --boundcond=Teff" )
            ( "kirr,k", po::value<double>(&k_irr)->default_value(k_irr), "[d log(z_0) / d log(r) - 1] factor for irradiation" )
			( "dir,d", po::value<string>(&output_dir)->default_value(output_dir), "Directory to write output files. It should exists" )
			( "F0,F", po::value<double>(&F0_gauss)->default_value(F0_gauss), "Initial viscous torque per radian on outer border of the disk, cgs" )
			( "initialcond,I", po::value<string>(&initial_cond_shape)->default_value(initial_cond_shape), "One of the available shapes of initial conditions for viscous torque F: sinusgauss, power, sinus" )
            ( "powerorder,p", po::value<double>(&power_order)->default_value(power_order), "Parameter of initial condition distribution: F ~ h^poweroder. This option works only with --initialcond=power" )
		;
		po::variables_map vm;

		try {
			po::store( po::parse_command_line(ac, av, desc), vm );
			po::notify(vm);
		} catch (exception &e){
			cerr << "Error: " << e.what() << endl;
			return 1;
		}

		if ( vm.count("help") ){
			cout << desc << endl;
			return 0;
		}
		output_fulldata = vm.count("fulldata");
		Mopt = vm["Mopt"].as<double>() * GSL_CONST_CGSM_SOLAR_MASS;
		Mx = vm["Mx"].as<double>() * GSL_CONST_CGSM_SOLAR_MASS;
		P = vm["period"].as<double>() * DAY;
        Distance = vm["distance"].as<double>() * kpc;
		nu_min = vm["numin"].as<double>() * keV;
		nu_max = vm["numax"].as<double>() * keV;
		tau = vm["tau"].as<double>() * DAY;
		Time = vm["time"].as<double>() * DAY;
		if ( not vm["rout"].defaulted() ){
			r_out = vm["rout"].as<double>() * solar_radius;
		}
		else{
			r_out = r_out_func( Mx, Mopt, P );
		}
        r_in = r_in_func( Mx, kerr );
	}

	const double GM = GSL_CONST_CGSM_GRAVITATIONAL_CONSTANT * Mx;
	const double h_in = sqrt( GSL_CONST_CGSM_GRAVITATIONAL_CONSTANT * Mx  * r_in );
	const double h_out = sqrt( GSL_CONST_CGSM_GRAVITATIONAL_CONSTANT * Mx  * r_out );
    const double cosiOverD2 = cosi / Distance / Distance;
//	cout
//        << "R_out = " << r_out / solar_radius << "\n"
//	    << "h_out = " << h_out << "\n"
//    	<< std::flush;

	auto wunc = [alpha, Mx, GM](
		const vector<double> &h, const vector<double> &F,
		int first, int last	
	) ->vector<double>{
		vector<double> W( first > 0 ? first : 0,  0. );
		for ( int i = first; i <= last; ++i ){
			W.push_back( 2.73e-9 * pow(F.at(i), 0.7) * pow(h.at(i), 0.8) * pow(alpha, -0.8) / GM ); //4. * Sigma * h*h*h / GMx / GMx, Sigma for free-free 1.2e25*rho/t^3.5, tau=100
//			W.push_back( 0.17 * pow(F.at(i), 2./3.) * h.at(i) * pow(alpha, -7./9.) * pow(Mx, -10/9) ); //4. * Sigma * h*h*h / GMx / GMx, Sigma for free-free 6.45e22*rho/t^2.5, tau=100
		}
		return W;
	};

    // Equation from Menou et al. 1999. Sigma_cr is from their fig 8 and connected to point where Mdot is minimal. Our Sigma is 0.5 from their Sigma.
    auto Sigma_hot_disk = [alpha, Mx](double r) ->double{
        return 0.5 * 39.9 * pow(alpha/0.1, -0.80) * pow(r/1e10, 1.11) * pow(Mx/GSL_CONST_CGSM_SOLAR_MASS, -0.37);
    };

	vector<double> h(Nx), R(Nx);
    for ( int i = 0; i < Nx; ++i ){
        if ( grid_scale == "log" ){ 
            h.at(i) = h_in * pow( h_out/h_in, i/(Nx-1.) );
        } else if ( grid_scale == "linear" ){
		    h.at(i) = h_in + (h_out - h_in) * i/(Nx-1.);
        } else{
            throw po::invalid_option_value(grid_scale);
        }
		R.at(i) = h.at(i) * h.at(i) / GM;
    }

    vector<double> F(Nx);
    if ( initial_cond_shape == "sinusgauss" ){
		const double F0_sinus = 1e-6 * F0_gauss;
		const double h_cut_for_F_gauss = h_out / sqrt(r_gauss_cut_to_r_out);
		const double F_gauss_cut = F0_gauss * exp( - (h_cut_for_F_gauss-h_out)*(h_cut_for_F_gauss-h_out) / (2. * h_out*h_out/(sigma_for_F_gauss*sigma_for_F_gauss)) );
		for ( int i = 0; i < Nx; ++i ){
			double F_gauss = F0_gauss * exp( - (h.at(i)-h_out)*(h.at(i)-h_out) / (2. * h_out*h_out/(sigma_for_F_gauss*sigma_for_F_gauss)) ) - F_gauss_cut;
			F_gauss = F_gauss >= 0 ? F_gauss : 0.;
			const double F_sinus =  F0_sinus * sin( (h.at(i) - h_in) / (h_out - h_in) * M_PI / 2. );
			F.at(i) = F_gauss + F_sinus;
		}
	} else if ( initial_cond_shape == "power" ){
        for ( int i = 0; i < Nx; ++i ){
            F.at(i) = F0_gauss * pow( (h.at(i) - h_in) / (h_out - h_in), power_order );
        }
    } else if ( initial_cond_shape == "sinus" ){
        for ( int i = 0; i < Nx; ++i ){
            F.at(i) = F0_gauss * sin( (h.at(i) - h_in) / (h_out - h_in) * M_PI / 2. );
        }
    } else if ( initial_cond_shape == "sinusparabola" ){
        const double h_F0 = h_out * 0.9;
        const double delta_h = h_out - h_F0;

        F0_gauss = 1.24e13 * pow(Sigma_hot_disk(R.at(Nx-1)), 10./7.) * pow(h.at(Nx-1), 22./7.) * pow(GM, -10./7.) * pow(alpha, 8./7.);

        Mdot_out = -kMdot_out * F0_gauss / (h_F0 - h_in) * M_PI*M_PI;

        for ( int i = 0; i < Nx; ++i ){
            if ( h.at(i) < h_F0 ){
                F.at(i) = F0_gauss * sin( (h.at(i) - h_in) / (h_F0 - h_in) * M_PI / 2. );
            } else{
                F.at(i) = F0_gauss * ( 1. - kMdot_out / (h_F0-h_in) / delta_h * M_PI / 4. * (h.at(i) - h_F0)*(h.at(i) - h_F0) );
            }
        }
    } else{
        throw po::invalid_option_value(initial_cond_shape);
    }	

	ofstream output_sum( output_dir + "/sum.dat" );
	output_sum << "#t	Mdot	Lx	H2R Rhot2Rout    Tphout kxout   mB  mV" << "\n";
    output_sum << "# r_out = " << r_out << "\n";
    output_sum << "#";
    for ( int i = 0; i < ac; ++i ){
        output_sum << " " << av[i];
    }
    output_sum << endl;

	for( double t = 0.; t <= Time; t += tau ){
		// cout << t/DAY << endl;

		vector<double> W(Nx, 0.), Tph_vis(Nx, 0.), Tph_X(Nx, 0.), Sigma(Nx, 0.), Height(Nx, 0.);
		
		try{
			nonlenear_diffusion_nonuniform_1_2 (tau, eps, 0., Mdot_out/(2.*M_PI), wunc, h, F);
			W = wunc(h, F, 1, Nx-1);
		} catch (runtime_error er){
			cout << er.what() << endl;
			break;
		}

		Mdot_in = 2.*M_PI * ( F.at(1) - F.at(0) ) / ( h.at(1) - h.at(0) );

		for ( int i = 1; i < Nx; ++i ){
			Sigma.at(i) = W.at(i) * GM*GM / (4. * pow(h.at(i), 3.));
			Height.at(i) = 6.4e4 * pow(F.at(i), 0.15) * pow(h.at(i), 2.1) * pow(alpha, -0.1) * pow(GM, -1.5); // for free-free 1.2e25*rho/t^3.5, tau=100
			Tph_vis.at(i) = GM * pow(h.at(i), -1.75) * pow( 0.75 * F.at(i) / GSL_CONST_CGSM_STEFAN_BOLTZMANN_CONSTANT, 0.25 );
			Tph_X.at(i) = fc * T_GR( R.at(i), 0., Mx, Mdot_in, R.front() );
		}
        
		const double Lx = Luminosity( R, Tph_X, nu_min, nu_max, 100 ) / pow(fc, 4.);

        auto Tph = vector<double>(Tph_vis);
        double k_x = 0.;
        int ii = Nx;
        if (bound_cond_type == "MdotOut"){
            Mdot_out = - kMdot_out * Mdot_in;
 
            do{
                ii--;
            } while( Sigma.at(ii) < Sigma_hot_disk(R[ii]) );

        } else if (bound_cond_type == "fourSigmaCrit"){
            do{
                ii--;
                // Equation from Menou et al. 1999. Factor 4 is from their fig 8 and connected to point where Mdot = 0. Our Sigma is 0.5 from their Sigma.
            } while( Sigma.at(ii) <  4 * Sigma_hot_disk(R[ii]) );
        } else if ( bound_cond_type == "Teff" ){
            if ( T_min_hot_disk > 0. ){
                do{
                    ii--;
                    k_x = k_irr * pow(Height.at(ii) / R.at(ii), 2.);
                    const double Qx = k_x * Mdot_in * GSL_CONST_CGSM_SPEED_OF_LIGHT * GSL_CONST_CGSM_SPEED_OF_LIGHT * eta / (4.*M_PI * R.at(ii)*R.at(ii));
                    Tph.at(ii) = pow( pow(Tph_vis.at(ii), 4.) + Qx / GSL_CONST_CGSM_STEFAN_BOLTZMANN_CONSTANT, 0.25 );
                } while( Tph.at(ii) < T_min_hot_disk );
            }
        } else{
            throw po::invalid_option_value(bound_cond_type);
        }

		const double mB = -2.5 * log10( I_lambda(R, Tph, lambdaB) * cosiOverD2 / irr0B );
		const double mV = -2.5 * log10( I_lambda(R, Tph, lambdaV) * cosiOverD2 / irr0V );

        if (output_fulldata){
			ostringstream filename;
			filename << output_dir << "/" << static_cast<int>(t/tau) << ".dat";
			ofstream output( filename.str() );
            output << "#h   F   Sigma   W   R   Tph_vis Height" << endl;
			for ( int i = 1; i < Nx; ++i ){
				output		<< h.at(i)
					<< "\t" << F.at(i)
					<< "\t" << Sigma.at(i)
					<< "\t" << W.at(i)
					<< "\t" << R.at(i)
					<< "\t" << Tph_vis.at(i)
					<< "\t" << Height.at(i)
					<< endl;
			}
		}

		output_sum		<< t / DAY
				<< "\t" << Mdot_in
				<< "\t" << Lx
				<< "\t" << Height.at(Nx-1) / R.at(Nx-1)
                << "\t" << R.at(Nx-1) / r_out
                << "\t" << Tph.at(Nx-1)
                << "\t" << k_x
                << "\t" << mB
                << "\t" << mV
				<< endl;

        if ( ii < Nx ){
            Nx = ii;
            // F.at(Nx-2) = F.at(Nx-1) - Mdot_out / (2.*M_PI) * (h.at(Nx-1) - h.at(Nx-2));
            h.resize(Nx);
        }
	}

	return 0;
}
