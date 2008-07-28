/* Shader
 *
 * Copyright (C) 2003-2005, Alexander Zaprjagaev <frustum@frustum.org>
 *                          Roman Grigoriev <grigoriev@gosniias.ru>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 */

#ifndef __SHADER_H__
#define __SHADER_H__

#include <simgear/compiler.h>
#include <simgear/screen/extensions.hxx>

#include <vector>

#include <map>
#include <string>

using std::map;
using std::vector;
using std::string;

class Shader {
public:
	
	Shader(const char *name,const char *vertex = NULL,const char *fragment = NULL);
	~Shader();
	
	void bindNames(const char *name,...);
	
	void enable();
	void disable();
	void bind();
	void bind(const float *value,...);
	
	void setLocalParameter(int location,const float *value);
	void setEnvParameter(int location,const float *value);
	
	void setParameter(const char *name,const float *value);
	void setParameters(const float *value,...);
	
	static void Init(void);
	inline static bool is_VP_supported(void) {return VP_supported;}
	inline static bool is_FP_supported(void) {return FP_supported;}
	inline static bool is_GLSL_supported(void) {return GLSL_supported;}
	inline static bool is_NVFP_supported(void) {return NVFP_supported;}
	inline static int get_nb_texture_units(void) {return nb_texture_unit;}

protected:
	
	struct Parameter {
		GLuint location;
		int length;
	};
	
	const char *get_error(char *data,int pos);
	const char *get_glsl_error();
	
	void getParameter(const char *name,Parameter *parameter);
	
	GLhandleARB program;
	
	GLuint vertex_target;
	GLuint vertex_id;
	
	GLuint fragment_target;
	GLuint fragment_id;
	
	std::vector<Parameter> parameters;
	typedef map<string, struct Parameter> arb_parameter_list;
	arb_parameter_list arb_parameters;

	static bool VP_supported, FP_supported, GLSL_supported, NVFP_supported;
	static GLint nb_texture_unit;
};

#endif /* __SHADER_H__ */
