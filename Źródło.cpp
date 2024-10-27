
// Nag³ówki
//#include "stdafx.h"
#include <GL/glew.h>
#include <SFML/Window.hpp>
#include <iostream>
#include <cmath>

// Kody shaderów
const GLchar* vertexSource = R"glsl(
#version 150 core
in vec2 position;
in vec3 color;
out vec3 Color;
void main(){
Color = color;
gl_Position = vec4(position, 0.0, 1.0);
}
)glsl";

const GLchar* fragmentSource = R"glsl(
#version 150 core
in vec3 Color;
out vec4 outColor;
void main()
{
outColor = vec4(Color, 1.0);
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
		std::cout << vertices[i * 6] <<"  " << vertices[i * 6 + 1] << std::endl;
	}
}

int main()
{
	sf::ContextSettings settings;
	settings.depthBits = 24;
	settings.stencilBits = 8;

	// Okno renderingu
	sf::Window window(sf::VideoMode(800, 600, 32), "OpenGL", sf::Style::Titlebar | sf::Style::Close, settings);

	// Inicjalizacja GLEW
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
	int ammVertices = 3;
	GLfloat vertices[ 15 * 6];
	std::cout << sizeof(vertices);
	setVerticies(*&vertices, ammVertices);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, ammVertices * 6 * sizeof(GLfloat), *&vertices, GL_DYNAMIC_DRAW);

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

	// Specifikacja formatu danych wierzcho³kowych
	GLint posAttrib = glGetAttribLocation(shaderProgram, "position");
	glEnableVertexAttribArray(posAttrib);
	glVertexAttribPointer(posAttrib, 2, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), 0);
	GLint colAttrib = glGetAttribLocation(shaderProgram, "color");
	glEnableVertexAttribArray(colAttrib);
	glVertexAttribPointer(colAttrib, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (void*)(3 * sizeof(GLfloat)));

	// Rozpoczêcie pêtli zdarzeñ
	bool running = true;
	while (running) {
		sf::Event windowEvent;
		while (window.pollEvent(windowEvent)) {
			switch (windowEvent.type) {
			case sf::Event::Closed:
				running = false;
				break;
			case sf::Event::KeyPressed:
				if (windowEvent.key.code >= sf::Keyboard::Num1 && windowEvent.key.code <= sf::Keyboard::Num9) {
					ammVertices = windowEvent.key.code - sf::Keyboard::Num1 + 1; // od 1 do 9
					setVerticies(vertices, ammVertices); // Zaktualizuj wierzcho³ki

					// Zaktualizuj VBO z nowymi danymi
					glBindBuffer(GL_ARRAY_BUFFER, vbo);
					glBufferData(GL_ARRAY_BUFFER, ammVertices * 6 * sizeof(GLfloat), vertices, GL_DYNAMIC_DRAW);
				}
				break;
			}

		}
		// Nadanie scenie koloru czarnego
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		// Narysowanie trójk¹ta na podstawie 3 wierzcho³ków
		glDrawArrays(GL_POLYGON, 0, ammVertices);
		// Wymiana buforów tylni/przedni
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