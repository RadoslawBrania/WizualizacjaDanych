
// Nag³ówki
//#include "stdafx.h"
#define POINTS 8
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <iostream>
#include <cmath>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <SFML/System/Time.hpp>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
// Kody shaderów
const GLchar* vertexSource = R"glsl(
#version 150 core
in vec3 position;
in vec3 color;
in vec3 aNormal;
in vec2 aTexCoord;
out vec3 Color;out vec2 TexCoord;
out vec3 Normal;
out vec3 FragPos;
uniform mat4 model;
uniform mat4 view;
uniform mat4 proj;
void main(){
	Color = color;
	TexCoord = aTexCoord;
	gl_Position = proj * view * model * vec4(position, 1.0);
	Normal = mat3(transpose(inverse(model))) * aNormal;
	FragPos = vec3(model * vec4(position, 1.0));	
}
)glsl";

const GLchar* fragmentSource = R"glsl(
#version 150 core
in vec3 Color;
in vec2 TexCoord;
in vec3 Normal;
in vec3 FragPos;
out vec4 outColor;
uniform sampler2D texture1;
uniform bool uLightingEnabled;  // Zmienna steruj¹ca oœwietleniem

uniform vec3 lightPos;
uniform vec3 viewPos;

uniform vec3 ambientLightColor;
uniform float ambientStrength;
uniform float lightIntensity;
uniform vec3 difflightColor;

uniform vec3 objectColor;

void main()
{
		// Oœwietlenie ambientowe
    // float ambientStrength = 0.1f;
    vec3 ambientLightColor = vec3(1.0, 1.0, 1.0);
    vec4 ambient = ambientStrength * vec4(ambientLightColor, 1.0);

	// Oœwietlenie rozproszone
    vec3 difflightColor = vec3(1.0, 1.0, 1.0);
    vec3 norm = normalize(Normal);
    vec3 lightDir = normalize(lightPos - FragPos);
    float diff = max(dot(norm, lightDir), 0.0);
    vec3 diffuse = diff * difflightColor * lightIntensity;

    if (uLightingEnabled)
    {
        //outColor = (ambient + vec4(diffuse, 1.0)) * texture(texture1, TexCoord);	
		outColor = (ambient + vec4(diffuse, 1.0)) * vec4(objectColor, 1.0);	
    }
    else
    {
        //outColor = texture(texture1, TexCoord);
		outColor = vec4(objectColor, 1.0);
    }
	
}
)glsl";
// Funkcja do sprawdzania kompilacji shaderów
void checkShaderCompilation(GLuint shader) {
	GLint success;
	glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
	if (!success) {
		GLint logLength;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
		char* infoLog = new char[logLength];
		glGetShaderInfoLog(shader, logLength, nullptr, infoLog);
		std::cerr << "Shader compilation error: " << infoLog << std::endl;
		delete[] infoLog;
	}
}

// Funkcja do sprawdzania linkowania programu shaderów
void checkProgramLinking(GLuint program) {
	GLint success;
	glGetProgramiv(program, GL_LINK_STATUS, &success);
	if (!success) {
		GLint logLength;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &logLength);
		char* infoLog = new char[logLength];
		glGetProgramInfoLog(program, logLength, nullptr, infoLog);
		std::cerr << "Program linking error: " << infoLog << std::endl;
		delete[] infoLog;
	}
}
	

void setVerticies(GLfloat *vertices, int ammVertices) {
	float radius = 0.5f;
	for (int i = 0; i < ammVertices+1; ++i) {
		float angle = 2.0f * 3.1415f * i / ammVertices;
		vertices[i * 6] = radius * cos(angle);
		vertices[i * 6 + 1] = radius * sin(angle);
		vertices[i * 6 + 2] = 0.0f;
		vertices[i * 6 + 3] = (float)i / ammVertices;
		vertices[i * 6 + 4] = 1.0f - (float)i / ammVertices;
		vertices[i * 6 + 5] = (float)(i % 2);
	}
}

unsigned int loadTexture(const std::string& texturePath) {
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_2D, textureID);

	// Ustawienie parametrów tekstury
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	// Wczytanie tekstury za pomoc¹ stb_image
	int width, height, nrChannels;
	stbi_set_flip_vertically_on_load(true);
	unsigned char* data = stbi_load(texturePath.c_str(), &width, &height, &nrChannels, 0);

	if (data) {
		// Elastyczne podejœcie zak³ada wariant dla nrChannels != 3
		GLenum format;
		if (nrChannels == 1)
			format = GL_RED;
		else if (nrChannels == 3)
			format = GL_RGB;
		else if (nrChannels == 4)
			format = GL_RGBA;

		glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data);
		glGenerateMipmap(GL_TEXTURE_2D);
	}
	else
	{
		std::cout << "Failed to load texture" << std::endl;

		stbi_image_free(data);

		return 0;
	}
	stbi_image_free(data);

	return textureID;
}



int main()
{
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;

	// Okno renderingu
	sf::Window window(sf::VideoMode(800, 600, 32), "OpenGL", sf::Style::Titlebar | sf::Style::Close, settings);
	window.setMouseCursorGrabbed(true);
	window.setMouseCursorVisible(false); 
	// Inicjalizacja GLEW
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	glewExperimental = GL_TRUE;
	glewInit();

	// Utworzenie VAO (Vertex Array Object)
	GLuint vao;
	glGenVertexArrays(1, &vao);
	glBindVertexArray(vao);

	// Utworzenie VBO (Vertex Buffer Object)
	// i skopiowanie do niego danych wierzcho³kowych
	GLuint vbo;
	glGenBuffers(1, &vbo);
	int ammVertices = 36;
	//GLfloat vertices[ 15 * 6];
	//setVerticies(*&vertices, ammVertices);
	/*GLfloat vertices[] = {
	-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,
	0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f,
	0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
	0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
	-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
	-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f,

	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
	0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f,
	-0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f,
	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,

	-0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
	-0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
	-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
	-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
	-0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,

	0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
	0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
	0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
	0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
	0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,

	-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
	0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
	0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
	0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
	-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f,

	-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f,
	0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f,
	-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f,
	-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f
	};
	*/
	GLfloat vertices[] = {
	-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.5f, -0.5f, -0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	-0.5f, -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	-0.5f, 0.5f, 0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,

	-0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	-0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	-0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

	0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,

	-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	0.5f, -0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.5f, -0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	-0.5f, -0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	-0.5f, -0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,

	-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f,
	0.5f, 0.5f, -0.5f, 1.0f, 1.0f, 0.0f, 1.0f, 1.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	0.5f, 0.5f, 0.5f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,
	-0.5f, 0.5f, 0.5f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f,
	-0.5f, 0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f
	};
	// Za³adowanie tekstury
	unsigned int texture1;
	glGenTextures(1, &texture1);
	glBindTexture(GL_TEXTURE_2D, texture1);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	loadTexture("bitmapbmp.bmp");

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, ammVertices * POINTS * sizeof(GLfloat), *&vertices, GL_STATIC_DRAW);


	// Utworzenie i skompilowanie shadera wierzcho³ków
	GLuint vertexShader =
		glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexSource, NULL);
	glCompileShader(vertexShader);

	// Utworzenie i skompilowanie shadera fragmentów
	GLuint fragmentShader =
		glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentSource, NULL);
	glCompileShader(fragmentShader);
	checkShaderCompilation(vertexShader);
	checkShaderCompilation(fragmentShader);
	// Zlinkowanie obu shaderów w jeden wspólny program
	GLuint shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glBindFragDataLocation(shaderProgram, 0, "outColor");
	glLinkProgram(shaderProgram);
	glUseProgram(shaderProgram);

	glm::mat4 model = glm::mat4(1.0f);
	model = glm::rotate(model, glm::radians(45.0f), glm::vec3(0.0f, 0.0f, 1.0f));

	glm::mat4 view;
	view = glm::lookAt(glm::vec3(0.0f, 0.0f, 3.0f),
		glm::vec3(0.0f, 0.0f, 0.0f),
		glm::vec3(0.0f, 1.0f, 0.0f));

	glm::mat4 proj = glm::perspective(glm::radians(45.0f), 800.0f / 800.0f, 0.06f, 100.0f);

	GLint uniTrans = glGetUniformLocation(shaderProgram, "model");
	glUniformMatrix4fv(uniTrans, 1, GL_FALSE, glm::value_ptr(model));

	GLint uniView = glGetUniformLocation(shaderProgram, "view");
	glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));

	GLint uniProj = glGetUniformLocation(shaderProgram, "proj");
	glUniformMatrix4fv(uniProj, 1, GL_FALSE, glm::value_ptr(proj));


	// Specifikacja formatu danych wierzcho³kowych
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 3, GL_FLOAT, GL_FALSE, POINTS * sizeof(GLfloat), 0);
	GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
	glEnableVertexAttribArray(colAttrib);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, POINTS * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));
	GLenum primitiveType = GL_TRIANGLES;

	// Textury
	GLint TexCoord = glGetAttribLocation(shaderProgram, "aTexCoord");
	glEnableVertexAttribArray(TexCoord);	glVertexAttribPointer(TexCoord, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(GLfloat), (void*)(6 * sizeof(GLfloat)));

	//lightning 
	glm::vec3 lightPos(1.2f, 1.5f, 2.0f);
	GLint uniLightPos = glGetUniformLocation(shaderProgram, "lightPos");
	glUniform3fv(uniLightPos, 1, &lightPos[0]);	bool lightingEnabled = true;  // Stan w³¹czenia oœwietlenia
	float ambientStrength = 0.1f;
	float lightIntensity = 1.0f;
	bool ambientOn = false;

	GLint lightingEnabledLocation = glGetUniformLocation(shaderProgram, "uLightingEnabled");
	GLint ambientStrengthLocation = glGetUniformLocation(shaderProgram, "ambientStrength");
	GLint lightIntensityLocation = glGetUniformLocation(shaderProgram, "lightIntensity");

	glUniform1i(lightingEnabledLocation, lightingEnabled);
	glUniform1f(ambientStrengthLocation, ambientStrength);
	glUniform1f(lightIntensityLocation, lightIntensity);
	// Rozpoczêcie pêtli zdarzeñ
	bool running = true;

	glm::vec3 cameraPos = glm::vec3(0.0f, 0.0f, 3.0f);
	glm::vec3 cameraFront = glm::vec3(0.0f, 0.0f, -1.0f);
	glm::vec3 cameraUp = glm::vec3(0.0f, 1.0f, 0.0f);
	sf::Clock clock;
	sf::Time time;
	double lastX = window.getSize().x / 2, lastY = window.getSize().y / 2;
	double yaw = -90.0, pitch = 0.0;
	float sensitivity = 0.005f;
	window.setFramerateLimit(60);
	float updateInterval = 0.2;
	float totalTime = 0;
	int frameCount = 0;
	float rotx = 0.0f;
	float roty = 0.0f;

	while (running) {
		float timeDiff = clock.restart().asSeconds();
		float baseSpeed = 0.5f;  // Podstawowa prêdkoœæ kamery
		float cameraSpeed = baseSpeed * timeDiff;
		totalTime += timeDiff;
		frameCount++;
		if (totalTime >= updateInterval) {
			window.setTitle("FPS: " + std::to_string(round(frameCount / totalTime)));
			totalTime = 0;
			frameCount = 0;
		}
		sf::Event windowEvent;
		while (window.pollEvent(windowEvent)) {
			switch (windowEvent.type) {
			case sf::Event::Closed:
				running = false;
				break;
			//case sf::Event::KeyPressed:
			//	if (windowEvent.key.code >= sf::Keyboard::Num1 && windowEvent.key.code <= sf::Keyboard::Num9) {
			//		if (windowEvent.key.code >= sf::Keyboard::Num1 && windowEvent.key.code <= sf::Keyboard::Num9) {
			//			int key = windowEvent.key.code - sf::Keyboard::Num1;
			//			GLenum types[] = { GL_POLYGON,GL_POINTS, GL_LINES, GL_LINE_STRIP, GL_LINE_LOOP, GL_TRIANGLES,GL_TRIANGLE_STRIP, GL_TRIANGLE_FAN, GL_QUADS, GL_QUAD_STRIP };
			//			primitiveType = types[key % 10]; // Zmiana prymitywu
			//		}
			//	}
			//	break;
			case sf::Event::KeyPressed:
				if (windowEvent.key.code == sf::Keyboard::Num1) {
					std::cout << "Brak" << std::endl;
					lightingEnabled = false;  
				}
				if (windowEvent.key.code == sf::Keyboard::Num2) {
					std::cout << "Punktowe" << std::endl;
					lightingEnabled = true;   
				}
				if (windowEvent.key.code == sf::Keyboard::Num3) {
					if (ambientOn) {
						ambientStrength += 0.1f;
						std::cout << "Obecna si³a otoczenia: " << ambientStrength << std::endl;
						glUniform1f(ambientStrengthLocation, ambientStrength);
					}
					else {
						lightIntensity += 0.1f;
						std::cout << "Obecna si³a punktowa: " << lightIntensity << std::endl;
						glUniform1f(lightIntensityLocation, lightIntensity);
					}

				}
				if (windowEvent.key.code == sf::Keyboard::Num4) {
					if (ambientOn) {
						ambientStrength -= 0.1f;
						if (ambientStrength < 0.0f) ambientStrength = 0.0f;  
						std::cout << "Zmniejszenie swiatla otoczenia " << ambientStrength << std::endl;
						glUniform1f(ambientStrengthLocation, ambientStrength);
					}
					else {
						lightIntensity -= 0.1f;
						if (lightIntensity < 0.0f) lightIntensity = 0.0f;  
						std::cout << "Zmniejszenie swiatla punktowego: " << lightIntensity << std::endl;
						glUniform1f(lightIntensityLocation, lightIntensity);
					}
				}
				if (windowEvent.key.code == sf::Keyboard::Num5) {
					lightPos = cameraPos;  
					uniLightPos = glGetUniformLocation(shaderProgram, "lightPos");
					glUniform3fv(uniLightPos, 1, &lightPos[0]);  
					std::cout << "Ustawiono nowa pozycje zrodla swiatla" << std::endl;
				}
				if (windowEvent.key.code == sf::Keyboard::Num6) {
					ambientOn = !ambientOn;
					if (ambientOn)
						std::cout << "Ustawiono tryb: zmiana swiatla otoczenia" << std::endl;
					else
						std::cout << "Ustawiono tryb: zmiana swiatla punktowego" << std::endl;
				}
				break;

			case sf::Event::MouseMoved:

				sf::Vector2i centerPosition(window.getSize().x / 2, window.getSize().y / 2);
				sf::Vector2i localPosition = sf::Mouse::getPosition(window);
				double xoffset = localPosition.x - centerPosition.x;
			//	double xoffset = localPosition.x - lastX;
				double yoffset = localPosition.y - centerPosition.y;
			//	double yoffset = localPosition.y - lastY;

				lastX = localPosition.x;
				lastY = localPosition.y;

				xoffset *= sensitivity;
				yoffset *= sensitivity;

				yaw += xoffset;
				pitch -= yoffset;


				if (pitch > 89.0f)
					pitch = 89.0f;
				if (pitch < -89.0f)
					pitch = -89.0f;
				sf::Mouse::setPosition(centerPosition, window);

				break;

			}

		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::W)) {
			cameraPos += cameraSpeed * cameraFront;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::S)) {
			cameraPos -= cameraSpeed * cameraFront;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::A)) {
			cameraPos -= glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::D)) {
			cameraPos += glm::normalize(glm::cross(cameraFront, cameraUp)) * cameraSpeed;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) {
			cameraPos.y += cameraSpeed;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::LShift)) {
			cameraPos.y -= cameraSpeed;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left)) {
			rotx -= cameraSpeed;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) {
			rotx += cameraSpeed;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up)) {
			roty += cameraSpeed;
		}
		if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down)) {
			roty -= cameraSpeed;
		}
		//int mouseY = sf::Mouse::getPosition(window).y;
		//ammVertices = std::max(3, int(10 * mouseY / window.getSize().y)%9);
		//setVerticies(*&vertices, ammVertices);
	/*	cameraFront.x = sin(rotx);
		cameraFront.z = -cos(rotx);
		cameraFront.y = sin(roty);*/
		glm::vec3 newFront;
		newFront.x = cos(glm::radians(yaw)) * cos(glm::radians(pitch));
		newFront.y = sin(glm::radians(pitch));
		newFront.z = sin(glm::radians(yaw)) * cos(glm::radians(pitch));
		cameraFront = glm::normalize(newFront);
		glm::mat4 view = glm::lookAt(cameraPos, cameraPos + cameraFront, cameraUp);


		GLint uniView = glGetUniformLocation(shaderProgram, "view");
		glUniformMatrix4fv(uniView, 1, GL_FALSE, glm::value_ptr(view));
		/*	glBindBuffer(GL_ARRAY_BUFFER, vbo);
			glBufferData(GL_ARRAY_BUFFER, ammVertices * 6 * sizeof(GLfloat), *&vertices, GL_STATIC_DRAW);*/
			// Nadanie scenie koloru czarnego
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		// Narysowanie trójk¹ta na podstawie 3 wierzcho³ków
		glDrawArrays(primitiveType, 0, ammVertices);
		// Wymiana buforów tylni/przedni
		glUniform1i(lightingEnabledLocation, lightingEnabled);
		window.display();
	}
	// Kasowanie programu i czyszczenie buforów
	glDeleteProgram(shaderProgram);
	glDeleteShader(fragmentShader);
	glDeleteShader(vertexShader);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	// Zamkniêcie okna renderingu
	window.close();
	return 0;
}