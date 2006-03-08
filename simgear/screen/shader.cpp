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

#ifdef HAVE_CONFIG_H
#  include <simgear_config.h>
#endif 

#include <simgear/debug/logstream.hxx>
#include "shader.h"
#include <stdio.h>
#include <stdarg.h>


glVertexAttrib1dProc glVertexAttrib1dPtr = NULL;
glVertexAttrib1dvProc glVertexAttrib1dvPtr = NULL;
glVertexAttrib1fProc glVertexAttrib1fPtr = NULL;
glVertexAttrib1fvProc glVertexAttrib1fvPtr = NULL;
glVertexAttrib1sProc glVertexAttrib1sPtr  = NULL;
glVertexAttrib1svProc glVertexAttrib1svPtr  = NULL;
glVertexAttrib2dProc glVertexAttrib2dPtr  = NULL;
glVertexAttrib2dvProc glVertexAttrib2dvPtr  = NULL;
glVertexAttrib2fProc glVertexAttrib2fPtr  = NULL;
glVertexAttrib2fvProc glVertexAttrib2fvPtr  = NULL;
glVertexAttrib2sProc glVertexAttrib2sPtr  = NULL;
glVertexAttrib2svProc glVertexAttrib2svPtr  = NULL;
glVertexAttrib3dProc glVertexAttrib3dPtr  = NULL;
glVertexAttrib3dvProc glVertexAttrib3dvPtr  = NULL;
glVertexAttrib3fProc glVertexAttrib3fPtr  = NULL;
glVertexAttrib3fvProc glVertexAttrib3fvPtr  = NULL;
glVertexAttrib3sProc glVertexAttrib3sPtr  = NULL;
glVertexAttrib3svProc glVertexAttrib3svPtr  = NULL;
glVertexAttrib4NbvProc glVertexAttrib4NbvPtr  = NULL;
glVertexAttrib4NivProc glVertexAttrib4NivPtr  = NULL;
glVertexAttrib4NsvProc glVertexAttrib4NsvPtr  = NULL;
glVertexAttrib4NubProc glVertexAttrib4NubPtr  = NULL;
glVertexAttrib4NubvProc glVertexAttrib4NubvPtr  = NULL;
glVertexAttrib4NuivProc glVertexAttrib4NuivPtr  = NULL;
glVertexAttrib4NusvProc glVertexAttrib4NusvPtr  = NULL;
glVertexAttrib4bvProc glVertexAttrib4bvPtr  = NULL;
glVertexAttrib4dProc glVertexAttrib4dPtr  = NULL;
glVertexAttrib4dvProc glVertexAttrib4dvPtr  = NULL;
glVertexAttrib4fProc glVertexAttrib4fPtr  = NULL;
glVertexAttrib4fvProc glVertexAttrib4fvPtr  = NULL;
glVertexAttrib4ivProc glVertexAttrib4ivPtr  = NULL;
glVertexAttrib4sProc glVertexAttrib4sPtr  = NULL;
glVertexAttrib4svProc glVertexAttrib4svPtr  = NULL;
glVertexAttrib4ubvProc glVertexAttrib4ubvPtr  = NULL;
glVertexAttrib4uivProc glVertexAttrib4uivPtr  = NULL;
glVertexAttrib4usvProc glVertexAttrib4usvPtr  = NULL;
glVertexAttribPointerProc glVertexAttribPointerPtr  = NULL;
glEnableVertexAttribArrayProc glEnableVertexAttribArrayPtr  = NULL;
glDisableVertexAttribArrayProc glDisableVertexAttribArrayPtr  = NULL;
glProgramStringProc glProgramStringPtr  = NULL;
glBindProgramProc glBindProgramPtr  = NULL;
glDeleteProgramsProc glDeleteProgramsPtr  = NULL;
glGenProgramsProc glGenProgramsPtr  = NULL;
glProgramEnvParameter4dProc glProgramEnvParameter4dPtr  = NULL;
glProgramEnvParameter4dvProc glProgramEnvParameter4dvPtr  = NULL;
glProgramEnvParameter4fProc glProgramEnvParameter4fPtr  = NULL;
glProgramEnvParameter4fvProc glProgramEnvParameter4fvPtr  = NULL;
glProgramLocalParameter4dProc glProgramLocalParameter4dPtr  = NULL;
glProgramLocalParameter4dvProc glProgramLocalParameter4dvPtr  = NULL;
glProgramLocalParameter4fProc glProgramLocalParameter4fPtr  = NULL;
glProgramLocalParameter4fvProc glProgramLocalParameter4fvPtr  = NULL;
glGetProgramEnvParameterdvProc glGetProgramEnvParameterdvPtr  = NULL;
glGetProgramEnvParameterfvProc glGetProgramEnvParameterfvPtr  = NULL;
glGetProgramLocalParameterdvProc glGetProgramLocalParameterdvPtr  = NULL;
glGetProgramLocalParameterfvProc glGetProgramLocalParameterfvPtr  = NULL;
glGetProgramivProc glGetProgramivPtr  = NULL;
glGetProgramStringProc glGetProgramStringPtr  = NULL;
glGetVertexAttribdvProc glGetVertexAttribdvPtr  = NULL;
glGetVertexAttribfvProc glGetVertexAttribfvPtr  = NULL;
glGetVertexAttribivProc glGetVertexAttribivPtr  = NULL;
glGetVertexAttribPointervProc glGetVertexAttribPointervPtr  = NULL;
glIsProgramProc glIsProgramPtr  = NULL;

glDeleteObjectProc glDeleteObjectPtr = NULL;
glGetHandleProc glGetHandlePtr = NULL;
glDetachObjectProc glDetachObjectPtr = NULL;
glCreateShaderObjectProc glCreateShaderObjectPtr = NULL;
glShaderSourceProc glShaderSourcePtr = NULL;
glCompileShaderProc glCompileShaderPtr = NULL;
glCreateProgramObjectProc glCreateProgramObjectPtr = NULL;
glAttachObjectProc glAttachObjectPtr = NULL;
glLinkProgramProc glLinkProgramPtr = NULL;
glUseProgramObjectProc glUseProgramObjectPtr = NULL;
glValidateProgramProc glValidateProgramPtr = NULL;
glUniform1fProc glUniform1fPtr = NULL;
glUniform2fProc glUniform2fPtr = NULL;
glUniform3fProc glUniform3fPtr = NULL;
glUniform4fProc glUniform4fPtr = NULL;
glUniform1iProc glUniform1iPtr = NULL;
glUniform2iProc glUniform2iPtr = NULL;
glUniform3iProc glUniform3iPtr = NULL;
glUniform4iProc glUniform4iPtr = NULL;
glUniform1fvProc glUniform1fvPtr = NULL;
glUniform2fvProc glUniform2fvPtr = NULL;
glUniform3fvProc glUniform3fvPtr = NULL;
glUniform4fvProc glUniform4fvPtr = NULL;
glUniform1ivProc glUniform1ivPtr = NULL;
glUniform2ivProc glUniform2ivPtr = NULL;
glUniform3ivProc glUniform3ivPtr = NULL;
glUniform4ivProc glUniform4ivPtr = NULL;
glUniformMatrix2fvProc glUniformMatrix2fvPtr = NULL;
glUniformMatrix3fvProc glUniformMatrix3fvPtr = NULL;
glUniformMatrix4fvProc glUniformMatrix4fvPtr = NULL;
glGetObjectParameterfvProc glGetObjectParameterfvPtr = NULL;
glGetObjectParameterivProc glGetObjectParameterivPtr = NULL;
glGetInfoLogProc glGetInfoLogPtr = NULL;
glGetAttachedObjectsProc glGetAttachedObjectsPtr = NULL;
glGetUniformLocationProc glGetUniformLocationPtr = NULL;
glGetActiveUniformProc glGetActiveUniformPtr = NULL;
glGetUniformfvProc glGetUniformfvPtr = NULL;
glGetUniformivProc glGetUniformivPtr = NULL;
glGetShaderSourceProc glGetShaderSourcePtr = NULL;

glBindAttribLocationProc glBindAttribLocationPtr = NULL;
glGetActiveAttribProc glGetActiveAttribPtr = NULL;
glGetAttribLocationProc glGetAttribLocationPtr = NULL;

glBindProgramNVProc glBindProgramNVPtr = NULL;
glDeleteProgramsNVProc glDeleteProgramsNVPtr = NULL;
glGenProgramsNVProc glGenProgramsNVPtr = NULL;
glLoadProgramNVProc glLoadProgramNVPtr = NULL;
glProgramParameter4fvNVProc glProgramParameter4fvNVPtr = NULL;

bool Shader::VP_supported = false;
bool Shader::FP_supported = false;
bool Shader::GLSL_supported = false;
bool Shader::NVFP_supported = false;
GLint Shader::nb_texture_unit = 0;

Shader::Shader(const char *name,const char *vertex,const char *fragment) {
	
	program = 0;
	vertex_target = 0;
	vertex_id = 0;
	fragment_target = 0;
	fragment_id = 0;
	
	char *data;
	FILE *file = fopen(name,"rb");
	if(!file) {
        SG_LOG(SG_GL, SG_ALERT, "Shader::Shader(): can't open '" << name << "' file\n");
		return;
	}
	
	fseek(file,0,SEEK_END);
	int size = ftell(file);
	data = new char[size + 1];
	data[size] = '\0';
	fseek(file,0,SEEK_SET);
	fread(data,1,size,file);
	fclose(file);
	
	// skip comments
	char *s = data;
	char *d = data;
	while(*s) {
		if(*s == '/' && *(s + 1) == '/') {
			while(*s && *s != '\n') s++;
			while(*s && *s == '\n') s++;
			*d++ = '\n';
		}
		else if(*s == '/' && *(s + 1) == '*') {
			while(*s && (*s != '*' || *(s + 1) != '/')) s++;
			s += 2;
			while(*s && *s == '\n') s++;
			*d++ = '\n';
		}
		else *d++ = *s++;
	}
	*d = '\0';
	
	// find shaders
	char *vertex_src = NULL;
	char *fragment_src = NULL;
	s = data;
	while(*s) {
		if(*s == '<') {
			char *name = s;
			while(*s) {
				if(strchr("> \t\n\r",*s)) break;
				s++;
			}
			if(*s == '>') {		// it`s shader
				*name++ = '\0';
				*s++ = '\0';
				while(*s && strchr(" \t\n\r",*s)) s++;
				if(vertex == NULL && !strcmp(name,"vertex")) vertex_src = s;
				if(vertex && !strcmp(name,vertex)) vertex_src = s;
				if(fragment == NULL && !strcmp(name,"fragment")) fragment_src = s;
				if(fragment && !strcmp(name,fragment)) fragment_src = s;
			}
		}
		s++;
	}
	
	if(vertex_src) {
		
		// ARB vertex program
		if(VP_supported && !strncmp(vertex_src,"!!ARBvp1.0",10)) {
			vertex_target = GL_VERTEX_PROGRAM_ARB;
			glGenProgramsPtr(1,&vertex_id);
			glBindProgramPtr(GL_VERTEX_PROGRAM_ARB,vertex_id);
			glProgramStringPtr(GL_VERTEX_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,(GLsizei)strlen(vertex_src),vertex_src);
			GLint pos = -1;
			glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&pos);
			if(pos != -1) {
				SG_LOG(SG_GL, SG_ALERT, "Shader::Shader(): vertex program error in " << name << " file\n" << get_error(vertex_src,pos));
				return;
			}
			char *var = strstr(vertex_src, "#var ");
			while( var ) {
				char *eol = strchr( var + 1, '#');
				char *c2, *c3, *c4;
				c2 = strchr( var + 6, ' ');
				if( c2 ) {
					c3 = strchr( c2 + 1, ':');
					if( c3 ) {
						c4 = strchr( c3 + 1, ':');
						if( c4 )
							c4 = strchr( c4 + 1, '[');
						if( c4 && c4 < eol) {
							char type[10], name[30];
							strncpy( type, var + 5, c2-var-5 );
							type[c2-var-5] = 0;
							strncpy( name, c2 + 1, c3-c2-2 );
							name[c3-c2-2] = 0;
							struct Parameter p;
							p.location = atoi( c4 + 1);
							p.length = 4;
							if( ! strcmp(type, "float3") )
								p.length = 3;
							else if( ! strcmp(type, "float") )
								p.length = 1;
							arb_parameters[ name ] = p;
						}
					}
				}
				var = strstr(var + 1, "#var ");
			}
		}
		// ARB vertex shader
		else {
		
			program = glCreateProgramObjectPtr();
			
			GLint length = (GLint)strlen(vertex_src);
			GLhandleARB vertex = glCreateShaderObjectPtr(GL_VERTEX_SHADER_ARB);
			glShaderSourcePtr(vertex,1,(const GLcharARB**)&vertex_src,&length);
			glCompileShaderPtr(vertex);
			glAttachObjectPtr(program,vertex);
			glDeleteObjectPtr(vertex);
			
			glBindAttribLocationPtr(program,0,"s_attribute_0");
			glBindAttribLocationPtr(program,1,"s_attribute_1");
			glBindAttribLocationPtr(program,2,"s_attribute_2");
			glBindAttribLocationPtr(program,3,"s_attribute_3");
			glBindAttribLocationPtr(program,4,"s_attribute_4");
			glBindAttribLocationPtr(program,5,"s_attribute_5");
			glBindAttribLocationPtr(program,6,"s_attribute_6");
			
			glBindAttribLocationPtr(program,0,"s_xyz");
			glBindAttribLocationPtr(program,1,"s_normal");
			glBindAttribLocationPtr(program,2,"s_tangent");
			glBindAttribLocationPtr(program,3,"s_binormal");
			glBindAttribLocationPtr(program,4,"s_texcoord");
		}
	}
	
	if(fragment_src) {
		
		// ARB fragment program
		if(FP_supported && !strncmp(fragment_src,"!!ARBfp1.0",10)) {
			fragment_target = GL_FRAGMENT_PROGRAM_ARB;
			glGenProgramsPtr(1,&fragment_id);
			glBindProgramPtr(GL_FRAGMENT_PROGRAM_ARB,fragment_id);
			glProgramStringPtr(GL_FRAGMENT_PROGRAM_ARB,GL_PROGRAM_FORMAT_ASCII_ARB,(GLsizei)strlen(fragment_src),fragment_src);
			GLint pos = -1;
			glGetIntegerv(GL_PROGRAM_ERROR_POSITION_ARB,&pos);
			if(pos != -1) {
				SG_LOG(SG_GL, SG_ALERT, "Shader::Shader(): fragment program error in " << name << " file\n" << get_error(fragment_src,pos));
				return;
			}
		}

		// NV fragment program
		else if(!strncmp(fragment_src,"!!FP1.0",7)) {
			fragment_target = GL_FRAGMENT_PROGRAM_NV;
			glGenProgramsNVPtr(1,&fragment_id);
			glBindProgramNVPtr(GL_FRAGMENT_PROGRAM_NV,fragment_id);
			glLoadProgramNVPtr(GL_FRAGMENT_PROGRAM_NV,fragment_id,(GLsizei)strlen(fragment_src),(GLubyte*)fragment_src);
			GLint pos = -1;
			glGetIntegerv(GL_PROGRAM_ERROR_POSITION_NV,&pos);
			if(pos != -1) {
				SG_LOG(SG_GL, SG_ALERT, "Shader::Shader(): fragment program error in " << name << " file\n" << get_error(fragment_src,pos));
				return;
			}
		}
		
		// ARB fragment shader
		else {
			
			if(!program) program = glCreateProgramObjectPtr();
			
			GLint length = (GLint)strlen(fragment_src);
			GLhandleARB fragment = glCreateShaderObjectPtr(GL_FRAGMENT_SHADER_ARB);
			glShaderSourcePtr(fragment,1,(const GLcharARB**)&fragment_src,&length);
			glCompileShaderPtr(fragment);
			glAttachObjectPtr(program,fragment);
			glDeleteObjectPtr(fragment);
		}
	}
	
	if(program) {

		glLinkProgramPtr(program);
		GLint linked;
		glGetObjectParameterivPtr(program,GL_OBJECT_LINK_STATUS_ARB,&linked);
		if(!linked) {
			SG_LOG(SG_GL, SG_ALERT, "Shader::Shader(): GLSL error in " << name << " file\n" << get_glsl_error());
			return;
		}
		
		glUseProgramObjectPtr(program);
		
		for(int i = 0; i < 8; i++) {
			char texture[32];
			sprintf(texture,"s_texture_%d",i);
			GLint location = glGetUniformLocationPtr(program,texture);
			if(location >= 0) glUniform1iPtr(location,i);
		}
		
		glUseProgramObjectPtr(0);
		
		glValidateProgramPtr(program);
		GLint validated;
		glGetObjectParameterivPtr(program,GL_OBJECT_VALIDATE_STATUS_ARB,&validated);
		if(!validated) {
			SG_LOG(SG_GL, SG_ALERT, "Shader::Shader(): GLSL error in " << name << " file\n" << get_glsl_error());
			return;
		}
	}
	
	delete [] data;
}

Shader::~Shader() {
	if(program) glDeleteObjectPtr(program);
	if(vertex_target == GL_VERTEX_PROGRAM_ARB) glDeleteProgramsPtr(1,&vertex_id);
	if(fragment_target == GL_FRAGMENT_PROGRAM_ARB) glDeleteProgramsPtr(1,&fragment_id);
	else if(fragment_target == GL_FRAGMENT_PROGRAM_NV) glDeleteProgramsNVPtr(1,&fragment_id);
	parameters.clear();
}

/*
 */
const char *Shader::get_error(char *data,int pos) {
	char *s = data;
	while(*s && pos--) s++;
	while(s >= data && *s != '\n') s--;
	char *e = ++s;
	while(*e != '\0' && *e != '\n') e++;
	*e = '\0';
	return s;
}

/*
 */

const char *Shader::get_glsl_error() {
	GLint length;
	static char error[4096];
	glGetInfoLogPtr(program,sizeof(error),&length,error);
	return error;
}

/*
 */
void Shader::getParameter(const char *name,Parameter *parameter) {
	if( program ) {
		char buf[1024];
		strcpy(buf,name);
		char *s = strchr(buf,':');
		if(s) {
			*s++ = '\0';
			parameter->length = atoi(s);
		} else {
			parameter->length = 4;
		}
		parameter->location = glGetUniformLocationPtr(program,buf);
	} else if( vertex_id ) {
		arb_parameter_list::iterator iParam = arb_parameters.find(name);
		if( iParam != arb_parameters.end() )
			parameter->location = iParam->second.location;
		else
			parameter->location = 90;
		parameter->length = 4;
	}
}

/*
 */
void Shader::bindNames(const char *name,...) {
	Parameter parameter;
	getParameter(name,&parameter);
	parameters.push_back(parameter);
	va_list args;
	va_start(args,name);
	while(1) {
		const char *name = va_arg(args,const char*);
		if(name == NULL) break;
		getParameter(name,&parameter);
		parameters.push_back(parameter);
	}
	va_end(args);
}

/*****************************************************************************/
/*                                                                           */
/* enable/disable/bind                                                       */
/*                                                                           */
/*****************************************************************************/

/*
 */
void Shader::enable() {
	if(vertex_id) glEnable(vertex_target);
	if(fragment_id) glEnable(fragment_target);
}

/*
 */
void Shader::disable() {
	if(program) glUseProgramObjectPtr(0);
	if(vertex_id) glDisable(vertex_target);
	if(fragment_id) glDisable(fragment_target);
}

/*
 */
void Shader::bind() {
	if(program) glUseProgramObjectPtr(program);
	if(vertex_id) {
		if(vertex_target == GL_VERTEX_PROGRAM_ARB) glBindProgramPtr(vertex_target,vertex_id);
	}
	if(fragment_id) {
		if(fragment_target == GL_FRAGMENT_PROGRAM_ARB) glBindProgramPtr(fragment_target,fragment_id);
		else if(fragment_target == GL_FRAGMENT_PROGRAM_NV) glBindProgramNVPtr(fragment_target,fragment_id);
	}
}

/*
 */
void Shader::bind(const float *v,...) {
	if(fragment_id) {
		if(fragment_target == GL_FRAGMENT_PROGRAM_ARB) glBindProgramPtr(fragment_target,fragment_id);
		else if(fragment_target == GL_FRAGMENT_PROGRAM_NV) glBindProgramNVPtr(fragment_target,fragment_id);
    } else {
	    if(program == 0) {
		    SG_LOG(SG_GL, SG_ALERT, "Shader::bind(): error GLSL shader isn't loaded\n");
		    return;
	    }
	    glUseProgramObjectPtr(program);
    }
	const float *value = v;
	va_list args;
	va_start(args,value);
	for(int i = 0; i < (int)parameters.size(); i++) {
		if( vertex_target ) {
			glProgramLocalParameter4fvPtr( vertex_target, parameters[i].location, value);
		} else if( program ) {
			if(parameters[i].length == 1) glUniform1fvPtr(parameters[i].location,1,value);
			else if(parameters[i].length == 2) glUniform2fvPtr(parameters[i].location,1,value);
			else if(parameters[i].length == 3) glUniform3fvPtr(parameters[i].location,1,value);
			else if(parameters[i].length == 4) glUniform4fvPtr(parameters[i].location,1,value);
			else if(parameters[i].length == 9) glUniformMatrix3fvPtr(parameters[i].location,1,false,value);
			else if(parameters[i].length == 16) glUniformMatrix4fvPtr(parameters[i].location,1,false,value);
		}
		value = va_arg(args,const float*);
		if(!value) break;
	}
	va_end(args);
}

/*****************************************************************************/
/*                                                                           */
/* set parameters                                                            */
/*                                                                           */
/*****************************************************************************/

void Shader::setLocalParameter(int location,const float *value) {
	if(vertex_target == 0) {
		SG_LOG(SG_GL, SG_ALERT, "Shader::setLocalParameter(): error vertex program isn't loaded\n");
		return;
	}
	glProgramLocalParameter4fvPtr(vertex_target,location,value);
}

void Shader::setEnvParameter(int location,const float *value) {
	if(vertex_target == 0) {
		SG_LOG(SG_GL, SG_ALERT, "Shader::setEnvParameter(): error vertex program isn't loaded\n");
		return;
	}
	glProgramEnvParameter4fvPtr(vertex_target,location,value);
}

/*
 */
void Shader::setParameter(const char *name,const float *value) {
	Parameter parameter;
	getParameter(name,&parameter);
	if( vertex_target ) {
		glProgramLocalParameter4fvPtr( vertex_target, parameter.location, value);
		return;
	}
	if(program == 0) {
		SG_LOG(SG_GL, SG_ALERT, "Shader::setLocalParameter(): error GLSL shader isn't loaded\n");
		return;
	}
	if(parameter.length == 1) glUniform1fvPtr(parameter.location,1,value);
	else if(parameter.length == 2) glUniform2fvPtr(parameter.location,1,value);
	else if(parameter.length == 3) glUniform3fvPtr(parameter.location,1,value);
	else if(parameter.length == 4) glUniform4fvPtr(parameter.location,1,value);
	else if(parameter.length == 9) glUniformMatrix3fvPtr(parameter.location,1,false,value);
	else if(parameter.length == 16) glUniformMatrix4fvPtr(parameter.location,1,false,value);
}

/*
 */
void Shader::setParameters(const float *v,...) {
	const float *value = v;
	va_list args;
	va_start(args,value);
	for(int i = 0; i < (int)parameters.size(); i++) {
		if( vertex_target ) {
			glProgramLocalParameter4fvPtr( vertex_target, parameters[i].location, value);
		} else if( program ) {
			if(parameters[i].length == 1) glUniform1fvPtr(parameters[i].location,1,value);
			else if(parameters[i].length == 2) glUniform2fvPtr(parameters[i].location,1,value);
			else if(parameters[i].length == 3) glUniform3fvPtr(parameters[i].location,1,value);
			else if(parameters[i].length == 4) glUniform4fvPtr(parameters[i].location,1,value);
			else if(parameters[i].length == 9) glUniformMatrix3fvPtr(parameters[i].location,1,false,value);
			else if(parameters[i].length == 16) glUniformMatrix4fvPtr(parameters[i].location,1,false,value);
		}
		value = va_arg(args,const float*);
		if(!value) break;
	}
	va_end(args);
}

#ifndef CONCAT
#define CONCAT(a,b) a##b
#endif
#define mystringify(a) CONCAT(ST,R)(a)
#define STR(x) #x
#define LOAD_EXT(fn) CONCAT(fn,Ptr) = (CONCAT(fn,Proc)) SGLookupFunction( mystringify(CONCAT(fn,ARB)) )

void Shader::Init(void) {
	if( SGIsOpenGLExtensionSupported("GL_ARB_multitexture") )
		glGetIntegerv( GL_MAX_TEXTURE_UNITS_ARB, &nb_texture_unit );
	VP_supported = SGIsOpenGLExtensionSupported("GL_ARB_vertex_program");
	FP_supported = SGIsOpenGLExtensionSupported("GL_ARB_fragment_program");
	// check that
	GLSL_supported = SGIsOpenGLExtensionSupported("GL_ARB_shading_language_100") &&
		SGIsOpenGLExtensionSupported("GL_ARB_fragment_shader") &&
		SGIsOpenGLExtensionSupported("GL_ARB_vertex_shader") &&
		SGIsOpenGLExtensionSupported("GL_ARB_shader_objects");
    NVFP_supported = SGIsOpenGLExtensionSupported("GL_NV_fragment_program");

	if( VP_supported || FP_supported ) {
		/* All ARB_fragment_program entry points are shared with ARB_vertex_program. */
        LOAD_EXT(glVertexAttrib1d);
        LOAD_EXT( glVertexAttrib1dv );
        LOAD_EXT( glVertexAttrib1f );
        LOAD_EXT( glVertexAttrib1fv );
        LOAD_EXT( glVertexAttrib1s );
        LOAD_EXT( glVertexAttrib1sv );
        LOAD_EXT( glVertexAttrib2d );
        LOAD_EXT( glVertexAttrib2dv );
        LOAD_EXT( glVertexAttrib2f );
        LOAD_EXT( glVertexAttrib2fv );
        LOAD_EXT( glVertexAttrib2s );
        LOAD_EXT( glVertexAttrib2sv );
        LOAD_EXT( glVertexAttrib3d );
        LOAD_EXT( glVertexAttrib3dv );
        LOAD_EXT( glVertexAttrib3f );
        LOAD_EXT( glVertexAttrib3fv );
        LOAD_EXT( glVertexAttrib3s );
        LOAD_EXT( glVertexAttrib3sv );
        LOAD_EXT( glVertexAttrib4Nbv );
        LOAD_EXT( glVertexAttrib4Niv );
        LOAD_EXT( glVertexAttrib4Nsv );
        LOAD_EXT( glVertexAttrib4Nub );
        LOAD_EXT( glVertexAttrib4Nubv );
        LOAD_EXT( glVertexAttrib4Nuiv );
        LOAD_EXT( glVertexAttrib4Nusv );
        LOAD_EXT( glVertexAttrib4bv );
        LOAD_EXT( glVertexAttrib4d );
        LOAD_EXT( glVertexAttrib4dv );
        LOAD_EXT( glVertexAttrib4f );
        LOAD_EXT( glVertexAttrib4fv );
        LOAD_EXT( glVertexAttrib4iv );
        LOAD_EXT( glVertexAttrib4s );
        LOAD_EXT( glVertexAttrib4sv );
        LOAD_EXT( glVertexAttrib4ubv );
        LOAD_EXT( glVertexAttrib4uiv );
        LOAD_EXT( glVertexAttrib4usv );
        LOAD_EXT( glVertexAttribPointer );
        LOAD_EXT( glEnableVertexAttribArray );
        LOAD_EXT( glDisableVertexAttribArray );
        LOAD_EXT( glProgramString );
        LOAD_EXT( glBindProgram );
        LOAD_EXT( glDeletePrograms );
        LOAD_EXT( glGenPrograms );
        LOAD_EXT( glProgramEnvParameter4d );
        LOAD_EXT( glProgramEnvParameter4dv );
        LOAD_EXT( glProgramEnvParameter4f );
        LOAD_EXT( glProgramEnvParameter4fv );
        LOAD_EXT( glProgramLocalParameter4d );
        LOAD_EXT( glProgramLocalParameter4dv );
        LOAD_EXT( glProgramLocalParameter4f );
        LOAD_EXT( glProgramLocalParameter4fv );
        LOAD_EXT( glGetProgramEnvParameterdv );
        LOAD_EXT( glGetProgramEnvParameterfv );
        LOAD_EXT( glGetProgramLocalParameterdv );
        LOAD_EXT( glGetProgramLocalParameterfv );
        LOAD_EXT( glGetProgramiv );
        LOAD_EXT( glGetProgramString );
        LOAD_EXT( glGetVertexAttribdv );
        LOAD_EXT( glGetVertexAttribfv );
        LOAD_EXT( glGetVertexAttribiv );
        LOAD_EXT( glGetVertexAttribPointerv );
        LOAD_EXT( glIsProgram );
	}
	if( GLSL_supported ) {
        LOAD_EXT( glDeleteObject );
        LOAD_EXT( glGetHandle );
        LOAD_EXT( glDetachObject );
        LOAD_EXT( glCreateShaderObject );
        LOAD_EXT( glShaderSource );
        LOAD_EXT( glCompileShader );
        LOAD_EXT( glCreateProgramObject );
        LOAD_EXT( glAttachObject );
        LOAD_EXT( glLinkProgram );
        LOAD_EXT( glUseProgramObject );
        LOAD_EXT( glValidateProgram );
        LOAD_EXT( glUniform1f );
        LOAD_EXT( glUniform2f );
        LOAD_EXT( glUniform3f );
        LOAD_EXT( glUniform4f );
        LOAD_EXT( glUniform1i );
        LOAD_EXT( glUniform2i );
        LOAD_EXT( glUniform3i );
        LOAD_EXT( glUniform4i );
        LOAD_EXT( glUniform1fv );
        LOAD_EXT( glUniform2fv );
        LOAD_EXT( glUniform3fv );
        LOAD_EXT( glUniform4fv );
        LOAD_EXT( glUniform1iv );
        LOAD_EXT( glUniform2iv );
        LOAD_EXT( glUniform3iv );
        LOAD_EXT( glUniform4iv );
        LOAD_EXT( glUniformMatrix2fv );
        LOAD_EXT( glUniformMatrix3fv );
        LOAD_EXT( glUniformMatrix4fv );
        LOAD_EXT( glGetObjectParameterfv );
        LOAD_EXT( glGetObjectParameteriv );
        LOAD_EXT( glGetInfoLog );
        LOAD_EXT( glGetAttachedObjects );
        LOAD_EXT( glGetUniformLocation );
        LOAD_EXT( glGetActiveUniform );
        LOAD_EXT( glGetUniformfv );
        LOAD_EXT( glGetUniformiv );
        LOAD_EXT( glGetShaderSource );

        LOAD_EXT( glBindAttribLocation );
        LOAD_EXT( glGetActiveAttrib );
        LOAD_EXT( glGetAttribLocation );

	}
    if( NVFP_supported ) {
        glBindProgramNVPtr = (glBindProgramNVProc) SGLookupFunction( "glBindProgramNV" );
        glDeleteProgramsNVPtr = (glDeleteProgramsNVProc) SGLookupFunction( "glDeleteProgramsNV" );
        glGenProgramsNVPtr = (glGenProgramsNVProc) SGLookupFunction( "glGenProgramsNV" );
        glLoadProgramNVPtr = (glLoadProgramNVProc) SGLookupFunction( "glLoadProgramNV" );
        glProgramParameter4fvNVPtr = (glProgramParameter4fvNVProc) SGLookupFunction( "glProgramParameter4fvNV" );
    }
}
