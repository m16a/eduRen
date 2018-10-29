#ifndef SHADER_H
#define SHADER_H

#include <glm/glm.hpp>

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <cassert>

class CShader
{
public:
    unsigned int ID;
    CShader(const char* vertexPath, const char* fragmentPath, const char* geometryPath = nullptr)
    {
        unsigned int vertex, fragment, geometry;
        vertex = loadShader(GL_VERTEX_SHADER, vertexPath);
        fragment = loadShader(GL_FRAGMENT_SHADER, fragmentPath);

        ID = glCreateProgram();
        glAttachShader(ID, vertex);
        glAttachShader(ID, fragment);

				if (geometryPath)
				{
					geometry = loadShader(GL_GEOMETRY_SHADER, geometryPath);
					glAttachShader(ID, geometry);
				}

        glLinkProgram(ID);
        checkCompileErrors(ID, "PROGRAM");
        // delete the shaders as they're linked into our program now and no longer necessery
        glDeleteShader(vertex);
        glDeleteShader(fragment);
        glDeleteShader(geometry);
    }
	
    // activate the shader
    // ------------------------------------------------------------------------
    void use() const
    { 
        glUseProgram(ID); 
    }
    // utility uniform functions
    // ------------------------------------------------------------------------
    void setBool(const std::string &name, bool value) const
    {         
        glUniform1i(glGetUniformLocation(ID, name.c_str()), (int)value); 
    }
    // ------------------------------------------------------------------------
    void setInt(const std::string &name, int value) const
    { 
        glUniform1i(glGetUniformLocation(ID, name.c_str()), value); 
    }
    // ------------------------------------------------------------------------
    void setFloat(const std::string &name, float value) const
    { 
        glUniform1f(glGetUniformLocation(ID, name.c_str()), value); 
    }
    // ------------------------------------------------------------------------
    void setVec2(const std::string &name, const glm::vec2 &value) const
    { 
        glUniform2fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec2(const std::string &name, float x, float y) const
    { 
        glUniform2f(glGetUniformLocation(ID, name.c_str()), x, y); 
    }
    // ------------------------------------------------------------------------
    void setVec3(const std::string &name, const glm::vec3 &value) const
    { 
        glUniform3fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec3(const std::string &name, float x, float y, float z) const
    { 
        glUniform3f(glGetUniformLocation(ID, name.c_str()), x, y, z); 
    }
    // ------------------------------------------------------------------------
    void setVec4(const std::string &name, const glm::vec4 &value) const
    { 
        glUniform4fv(glGetUniformLocation(ID, name.c_str()), 1, &value[0]); 
    }
    void setVec4(const std::string &name, float x, float y, float z, float w) const
    { 
        glUniform4f(glGetUniformLocation(ID, name.c_str()), x, y, z, w); 
    }
    // ------------------------------------------------------------------------
    void setMat2(const std::string &name, const glm::mat2 &mat) const
    {
        glUniformMatrix2fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat3(const std::string &name, const glm::mat3 &mat) const
    {
        glUniformMatrix3fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
    void setMat4(const std::string &name, const glm::mat4 &mat) const
    {
        glUniformMatrix4fv(glGetUniformLocation(ID, name.c_str()), 1, GL_FALSE, &mat[0][0]);
    }
    // ------------------------------------------------------------------------
		
		using TSubroutineTypeToInstance = std::vector<std::pair<std::string, std::string>>;
		void setSubroutine(GLenum programType, const TSubroutineTypeToInstance& data)
		//void setSubroutine(GLenum programType, const std::string &subType, const std::string &subInstance)
		{
			GLint subUniformsCount = 0;
			glGetProgramStageiv(ID, programType, GL_ACTIVE_SUBROUTINE_UNIFORM_LOCATIONS, &subUniformsCount);
			//std::cout << subUniformsCount << std::endl;
			assert(subUniformsCount && "no subroutine uniforms");
			//assert(subUniformsCount == data.size() && "subroutime uniforms count mismatch");
			std::vector<GLuint> indices(subUniformsCount);
			
			for ( auto& p : data)
			{
				GLint selectorLoc = glGetSubroutineUniformLocation(ID, programType, p.first.c_str());
				GLuint index = glGetSubroutineIndex(ID, programType, p.second.c_str());
				if (selectorLoc <= -1)
				{
					std::cout << p.first << ":" << selectorLoc << " " << p.second << ":" << index << std::endl;
					assert(0 && "bad subroutine uniform location");
				}

				assert(index != GL_INVALID_INDEX && "bad subroutine index");

				indices[selectorLoc] = index;
			}
			

			glUniformSubroutinesuiv(programType, subUniformsCount, indices.data());
		}

	private:
		std::string shaderTypeToStr(GLenum type)
		{
			switch (type)
			{
				case GL_VERTEX_SHADER:
					return "VERTEX";
					break;
				case GL_FRAGMENT_SHADER:
					return "FRAGMENT";
					break;
				case GL_GEOMETRY_SHADER:
					return "GEOMETRY";
					break;
				default:
					assert(0 && "no shader type str");
					break;
			}
		}

		GLuint loadShader(GLenum type, const char* path)
		{
			GLuint res = 0;

			// 1. retrieve the source code from filePath
			std::string code;
			std::ifstream file;
			
			// ensure ifstream objects can throw exceptions:
			file.exceptions (std::ifstream::failbit | std::ifstream::badbit);
			try 
			{
					// open files
					file.open(path);
					std::stringstream stream;
					// read file's buffer contents into streams
					stream << file.rdbuf();
					// close file handlers
					file.close();
					// convert stream into string
					code = stream.str();
			}
			catch (std::ifstream::failure e)
			{
					std::cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << std::endl;
			}
			const char* code_str = code.c_str();
			//
			// 2. compile shaders
			unsigned int vertex, fragment;
			int success;
			char infoLog[512];
			// vertex shader
			res = glCreateShader(type);
			glShaderSource(res, 1, &code_str, NULL);
			glCompileShader(res);
			checkCompileErrors(res, shaderTypeToStr(type), path);

			return res;
		}

    // utility function for checking shader compilation/linking errors.
    // ------------------------------------------------------------------------
    void checkCompileErrors(GLuint shader, std::string type, const char* path = nullptr)
    {
        GLint success;
        GLchar infoLog[1024];
        if (type != "PROGRAM")
        {
            glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
            if (!success)
            {
                glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR:(" << path << "):SHADER_COMPILATION_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
								exit(1);
            }
        }
        else
        {
            glGetProgramiv(shader, GL_LINK_STATUS, &success);
            if (!success)
            {
                glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                std::cout << "ERROR::PROGRAM_LINKING_ERROR of type: " << type << "\n" << infoLog << "\n -- --------------------------------------------------- -- " << std::endl;
								exit(1);
            }
        }
    }
};
#endif
