/*
  ===========================================================================

  Copyright (C) 2014 Emvivre

  This file is part of MRV2PLY.

  MRV2PLY is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  MRV2PLY is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with MRV2PLY.  If not, see <http://www.gnu.org/licenses/>.

  ===========================================================================
*/

#include <iostream>
#include <fstream>
#include <sstream>
#include "regex.hh"
#include <map>
#include "string_util.h"
#include <vector>
#include <array>
#include <limits>
#include <glm/glm.hpp>

#define GLM_FORCE_RADIANS
#include <glm/gtx/rotate_vector.hpp>

class EdgeParsingError {};
class AtomWithoutEdge {};
class UnableToFindAtomNameInColormap {};

struct Atom {
	std::string real_name;
	double x, y, z;
	Atom() {}
	Atom(std::string real_name, double x, double y, double z) : real_name(real_name), x(x), y(y), z(z) {}
	friend std::ostream& operator<<(std::ostream& os, const Atom& a) {
		return os << a.real_name << " [ " << a.x << ", " << a.y << ", " << a.z << " ]";
	}
};

struct Molecule
{
	typedef std::pair<std::string, std::string> Edge;
	std::map<std::string, Atom> atoms;
	std::vector<Edge> edges;	
	friend std::ostream& operator<<(std::ostream& os, const Molecule& m) {
		os << "nb_atoms: " << m.atoms.size() << "\n";
		for ( const auto& e : m.atoms ) {
			os << e.first << ": " << e.second << "\n";
		}
		os << "edges:\n";
		for ( const auto& e : m.edges ) {
			os << "  " << e.first << " - " << e.second << "\n";
		}
		return os;
	}
};

std::map<std::string, glm::vec3> colormap = { 
	{ "H", glm::vec3(1, 1, 1) },
        { "C", glm::vec3(0.3, 0.3, 0.3) },
	{ "O", glm::vec3(1, 0, 0) },
	{ "N", glm::vec3(0, 0, 1) },
	{ "F", glm::vec3(0, 1, 0) },
	{ "Cl", glm::vec3(0, 1, 1) },
	{ "Br", glm::vec3(1, 0, 1) },
};

int main(int argc, char** argv)
{
	if ( argc < 9 ) {
		std::cout << 
			"Usage: " << argv[0] << " <MRV_FILE> <EDGE_THICKNESS> <EDGE_SUBDIVISION> <SPHERE_RADIUS> <SPHERE_SUBDIVISION_LONGITUDE> <SPHERE_SUBDIVISION_LATITUDE> <HYDRONGENE_RADIUS_SCALE> <PLY_MESH_OUTPUT>\n"
			"   ex: " << argv[0] << " 1.mrv 0.1 20 0.3 20 20 1 molecule.ply\n"
			"   ex: " << argv[0] << " 1.mrv 0.1 20 0.4 20 20 0.5 molecule.ply\n";		   
		return 1;
	}
	int i = 0;
	std::string mrv_input = argv[++i]; std::cout << "mrv_input: " << mrv_input << "\n";
	float edge_thickness = atof(argv[++i]); std::cout << "edge_thickness: " << edge_thickness << "\n";
	int edge_subdivision = atoi(argv[++i]); std::cout << "edge_subdivision: " << edge_subdivision << "\n";
	float sphere_radius = atof(argv[++i]); std::cout << "sphere_radius: " << sphere_radius << "\n";
	int sphere_subdivision_longitude = atoi(argv[++i]); std::cout << "sphere_subdivision_longitude: " << sphere_subdivision_longitude << "\n";
	int sphere_subdivision_latitude = atoi(argv[++i]); std::cout << "sphere_subdivision_latitude: " << sphere_subdivision_latitude << "\n";	
	float hydrogene_radius_scale = atof(argv[++i]); std::cout << "hydrogene_radius_scale: " << hydrogene_radius_scale << "\n";
	std::string ply_out = argv[++i]; std::cout << "ply_out: " << ply_out << "\n";

	std::cout << "Reading mrv file...\n";
	std::ifstream mrv(mrv_input);
	std::string l;
	Molecule m;
	std::vector<std::string> atomID;
	std::vector<std::string> elementType;
	std::vector<std::string> x3;
	std::vector<std::string> y3;
	std::vector<std::string> z3;
	while ( std::getline(mrv, l) ) {
		std::vector<std::string> r;
		
		if ( (r = Regex::search("<atomArray atomID=\"([^\"]+)\" elementType=\"([^\"]+)\" x3=\"([^\"]+)\" y3=\"([^\"]+)\" z3=\"([^\"]+)\"></atomArray>", l)).size() == 5 ) {
			atomID = StringUtil::split(r[0], ' ');
			elementType = StringUtil::split(r[1], ' ');
			x3 = StringUtil::split(r[2], ' ');
			y3 = StringUtil::split(r[3], ' ');
			z3 = StringUtil::split(r[4], ' ');
		}
		if ( (r = Regex::search("<bond atomRefs2=\"([^\"]+)\" order=\".\"></bond>", l)).size() > 0 ) {
			for ( const auto& e : r ) {
				std::vector<std::string> atoms = Regex::search("(.*) (.*)", e);
				if ( atoms.size() != 2 ) throw EdgeParsingError();
				std::string a0 = atoms[0];
				std::string a1 = atoms[1];
				m.edges.push_back( std::make_pair(a0, a1) );
			}
		}
	}
	unsigned int nb_atoms = atomID.size();
	for ( unsigned int i = 0; i < nb_atoms; i++ ) {
		Atom a( elementType[i], atof(x3[i].c_str()), atof(y3[i].c_str()), atof(z3[i].c_str()) );
		m.atoms[atomID[i]] = a;
	}
	std::cout << m << "\n";

	std::cout << "Converting to a 3D triangular model...\n";
	std::vector<glm::vec3> pts;
	std::vector<glm::vec3> colors;
	std::vector<std::array<unsigned int,3> > triangles;

	std::vector<glm::vec3> sphere_pts;
	/**
	 *  x <- r * sin(\alpha) * cos(\beta)
	 *  y <- r * sin(\alpha) * sin(\beta)
	 *  z <- r * cos(\alpha)
	 * avec:
	 *   \alpha \in [0, pi] : latitude
	 *   \beta \in [0, 2*pi] : longitude
	 */ 
	for ( int la = 0; la < sphere_subdivision_latitude; la++ ) {
		float alpha = (M_PI * la) / (sphere_subdivision_latitude-1);
		float sa = std::sin(alpha);
		float ca = std::cos(alpha);
		for ( int lo = 0; lo < sphere_subdivision_longitude; lo++) {
			float beta = (2 * M_PI * lo) / sphere_subdivision_longitude;
			float x = sphere_radius * sa * std::cos(beta);
			float y = sphere_radius * sa * std::sin(beta);
			float z = sphere_radius * ca;
			glm::vec3 p(x, y, z);
			sphere_pts.push_back(p);
		}            
	}
	std::vector<std::array<unsigned int,3> > sphere_triangles;
	for ( int la = 0; la < sphere_subdivision_latitude; la++ ) {
		for ( int lo = 0; lo < sphere_subdivision_longitude; lo++ ) {                
			int lo_next = (lo+1)%sphere_subdivision_longitude;
			int la_next = (la+1)%sphere_subdivision_latitude;
			unsigned int v0 = la*sphere_subdivision_longitude+lo;
			unsigned int v1 = la*sphere_subdivision_longitude+lo_next;
			unsigned int v2 = la_next*sphere_subdivision_longitude+lo;
			unsigned int v3 = la_next*sphere_subdivision_longitude+lo_next;
			sphere_triangles.push_back( {{ v0, v2, v1 }} );
			sphere_triangles.push_back( {{ v1, v2, v3 }} );
		}
	}		
	unsigned int atom_id = 0;	
	const glm::vec3 x_axis(1, 0, 0);
	const glm::vec3 y_axis(0, 1, 0);
	const glm::vec3 z_axis(0, 0, 1);
	for ( const auto& e : m.atoms ) {
		const Atom& a = e.second;
		const Atom* a_dst = NULL;
		for ( const auto& e : m.edges ) {
			if ( m.atoms[e.first].real_name == a.real_name ) {
				a_dst = &m.atoms[e.second];
				break;
			}
			if ( m.atoms[e.second].real_name == a.real_name ) {
				a_dst = &m.atoms[e.first];
				break;
			}				
		}
		if (a_dst == NULL) { 
			std::cout << "ERROR ===> atom: " << e.first << " - [" << m.atoms[e.first].real_name << "] is not linked WTF ?!?!\n";
			throw AtomWithoutEdge();
		}
		glm::vec3 sphere_center = glm::vec3(a.x, a.y, a.z);
		if ( colormap.count(a.real_name) == 0 ) throw UnableToFindAtomNameInColormap();
		glm::vec3 c = colormap[a.real_name];
		atom_id++;
		unsigned int sphere_pt_beg_i = pts.size();
		float radius_scale = (a.real_name == "H") ? hydrogene_radius_scale : 1;
		for ( const auto& p : sphere_pts ) {
			glm::vec3 pp = radius_scale * p;
			pp += sphere_center;			
			pts.push_back(pp);
			colors.push_back(c);
		}
		for ( const auto& t : sphere_triangles ) {
			unsigned int i0 = sphere_pt_beg_i + t[0];
			unsigned int i1 = sphere_pt_beg_i + t[1];
			unsigned int i2 = sphere_pt_beg_i + t[2];
			triangles.push_back( {{ i0, i1, i2 }} );
		}
	}	
	for ( const auto& e : m.edges ) {
		const Atom& a_begin = m.atoms[ e.first ];
		const Atom& a_end = m.atoms[ e.second ];
		glm::vec3 edge_begin = glm::vec3(a_begin.x, a_begin.y, a_begin.z);
		glm::vec3 edge_end = glm::vec3(a_end.x, a_end.y, a_end.z);
		glm::vec3 dir = glm::normalize(edge_end - edge_begin);
		glm::vec3 v_ref = (dir != x_axis) ? x_axis : y_axis;
		glm::vec3 t_ref = glm::normalize( glm::cross(dir, v_ref) );
		glm::vec3 t = t_ref;		
		unsigned int pts_beg_i = pts.size();
		for ( int i = 0; i < edge_subdivision-1; i++ ) {
			float angle = (i+1) * 2 * M_PI / edge_subdivision;
			glm::mat4 m = glm::rotate(angle, dir);
			glm::vec4 tt4 = m * glm::vec4(t_ref, 1);
			glm::vec3 tt = glm::vec3(tt4.x, tt4.y, tt4.z);
			glm::vec3 p0 = edge_begin + edge_thickness * t;
			glm::vec3 p1 = edge_end + edge_thickness * t;
			glm::vec3 p00 = edge_begin + edge_thickness * tt;
			glm::vec3 p11 = edge_end + edge_thickness * tt;
			unsigned int p0_i = pts.size();
			pts.push_back(p0); colors.push_back(glm::vec3(1,1,1));
			pts.push_back(p1); colors.push_back(glm::vec3(1,1,1));
			pts.push_back(p00); colors.push_back(glm::vec3(1,1,1));
			pts.push_back(p11); colors.push_back(glm::vec3(1,1,1));
			triangles.push_back( {{ p0_i, p0_i+2, p0_i+1 }} );
			triangles.push_back( {{ p0_i+1, p0_i+2, p0_i+3 }} );
			t = tt;
		}				
		unsigned int pts_end_i = pts.size()-2;
		triangles.push_back( {{ pts_end_i, pts_beg_i, pts_end_i+1 }} );
		triangles.push_back( {{ pts_end_i+1, pts_beg_i, pts_beg_i+1 }} );
	}
	
	
	std::cout << "Exporting to ply...\n";
	std::ofstream ply_out_fd (ply_out);
	ply_out_fd << 
		"ply\n"
		"format ascii 1.0\n"
		"comment mrv2ply\n"
		"element vertex " << pts.size() << "\n"
		"property float x\n"
		"property float y\n"
		"property float z\n"
		"property uchar red\n"
		"property uchar green\n"
		"property uchar blue\n"
		"element face " << triangles.size() << "\n"
		"property list uchar int vertex_indices\n"
		"end_header\n";
	for ( unsigned int i = 0; i < pts.size(); i++ ) {
		glm::vec3 p = pts[i];
		glm::vec3 c = 255.f * colors[i];
		ply_out_fd << p.x << " " << p.y << " " << p.z << " " << int(c.r) << " " << int(c.g) << " " << int(c.b) << "\n";
	}
	for ( const auto& t : triangles ) {
		ply_out_fd << "3 " << t[0] << " " << t[1] << " " << t[2] << "\n";
	}
	return 0; 
}
