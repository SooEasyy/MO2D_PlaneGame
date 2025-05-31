#include <iostream> 
#include <cstdlib> 
#include <cmath> 
#include <GL/glut.h>
#include "Teclado.h"
#include "uglyfont.h"
#include <vector>

using namespace std;

//------------------------------------------------------------
// variables globales
int w = 800, h = 600; // tamaño inicial de la ventana

double AvionX = 400, AvionY = 300, AvionAng = 0, ArmaAng = 0, ArmaTamanio = 0;

const double PI = 4 * atan(1.0);
bool cl_info = true; // informa por la línea de comandos
const double VELOCIDAD = 2.0; // Velocidad de movimiento del avión
const double GRAVEDAD = 0.1;  // Simulación de gravedad (si la quieres aplicar)
GLuint texturaFondo;          // Identificador para la textura de fondo
GLuint texturaJupiter;        // Identificador para la textura de Jupiter
GLuint texturaGalaxia;           // Identificador para la textura de Luna
bool teclas[256] = { false }; // Arreglo para registrar teclas activas
int vidasAvion = 3;             // Número de vidas del avión
bool juegoActivo = true;        // Indica si el juego sigue o terminó
int contadorDisparo = 0; // Contador de disparos
const int intervaloDisparo = 60; // Cada 60 frames
int puntaje = 0; // Contador de puntaje
int nivel = 1; // Numero de nivel
const int nivelMaximo = 10; // Nivel maximo 10
bool victoria = false;
// TURBO
bool turboActivo = false;
int framesTurboRestantes = 0;
int framesCooldownRestante = 0;
const int DURACION_TURBO = 300;      // 5 segundos (60 FPS x 5)
const int ENFRIAMIENTO_TURBO = 1800; // 30 segundos (60 FPS x 30)
double VELOCIDAD_BASE = 2.0;
double VELOCIDAD_TURBO = 5.0;
double velocidadActual = turboActivo ? VELOCIDAD_TURBO : VELOCIDAD_BASE;

//============================================================
// Función para cargar una textura PPM
GLuint CargarTexturaPPM(const char* filename) {
    FILE* fp = fopen(filename, "rb");
    if (!fp) {
        cerr << "Error: No se pudo abrir el archivo " << filename << endl;
        exit(1);
    }

    // Leer el encabezado PPM
    char format[3];
    int width, height, maxVal;
    fscanf(fp, "%s\n%d %d\n%d\n", format, &width, &height, &maxVal);
    if (strcmp(format, "P6") != 0) {
        cerr << "Error: Formato PPM no soportado." << endl;
        fclose(fp);
        exit(1);
    }

    // Leer los datos de píxeles
    unsigned char* data = new unsigned char[width * height * 4]; 
    fread(data, sizeof(unsigned char), width * height * 4, fp);
    fclose(fp);

    // Generar una textura de OpenGL
    GLuint textureID;
    glGenTextures(1, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    // Configurar parámetros de textura
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    //Cargar la textura
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);

    delete[] data;  // Liberar la memoria

    return textureID;
}

// Estructura y almacenamiento de los proyectiles
struct Proyectil {
    double x, y;  // Posición
    double velX, velY;  // Velocidad en X y Y
    double angulo;  // Ángulo de disparo
};

struct ProyectilEnemigo {
    double x, y; // Posición
    double velX, velY; // Velocidad en X y Y
};

std::vector<ProyectilEnemigo> proyectilesEnemigos;
const double VELOCIDAD_PROYECTIL_ENEMIGO = 2.0; // Velocidad del proyectil

std::vector<Proyectil> proyectiles;
const double VELOCIDAD_PROYECTIL = 5.0;  // Velocidad del proyectil

// Estructura del enemigo
struct Enemigo {
    double x, y;  // Posición del enemigo
    double radio;  // Radio del círculo enemigo
    int vidaMax;  // Vida máxima del enemigo
    int vidaActual;  // Vida actual del enemigo
};
Enemigo enemigo = { 600, 300, 50, 100, 100 };  // Inicialización del enemigo

void CrearProyectiles() {
    double anguloRad = AvionAng * PI / -180.0;  // Convertir ángulo a radianes
    double offsetFrontal = 20.0;  // Distancia hacia adelante (desde la nariz del avión)
    double offsetLateral = 25.0;  // Separación entre metrallas

    // Calcular desplazamiento perpendicular (hacia los lados)
    double perpendicularX = offsetLateral * cos(anguloRad + PI / 2);
    double perpendicularY = offsetLateral * sin(anguloRad + PI / 2);

    // Proyectil izquierdo
    Proyectil proyectilIzquierdo;
    proyectilIzquierdo.x = AvionX + offsetFrontal * sin(anguloRad) - perpendicularX;
    proyectilIzquierdo.y = AvionY - offsetFrontal * cos(anguloRad) - perpendicularY;
    proyectilIzquierdo.velX = VELOCIDAD_PROYECTIL * sin(anguloRad);
    proyectilIzquierdo.velY = VELOCIDAD_PROYECTIL * cos(anguloRad);
    proyectilIzquierdo.angulo = AvionAng;

    // Proyectil derecho
    Proyectil proyectilDerecho;
    proyectilDerecho.x = AvionX + offsetFrontal * sin(anguloRad) + perpendicularX;
    proyectilDerecho.y = AvionY - offsetFrontal * cos(anguloRad) + perpendicularY;
    proyectilDerecho.velX = VELOCIDAD_PROYECTIL * sin(anguloRad);
    proyectilDerecho.velY = VELOCIDAD_PROYECTIL * cos(anguloRad);
    proyectilDerecho.angulo = AvionAng;

    // Agregar proyectiles a la lista
    proyectiles.push_back(proyectilIzquierdo);
    proyectiles.push_back(proyectilDerecho);
}

void DispararDesdeEnemigo() {
    if (enemigo.vidaActual <= 0) return;

    double dx = AvionX - enemigo.x;
    double dy = AvionY - enemigo.y;
    double distancia = sqrt(dx * dx + dy * dy);

    if (distancia == 0) return;

    double vx = VELOCIDAD_PROYECTIL_ENEMIGO * dx / distancia;
    double vy = VELOCIDAD_PROYECTIL_ENEMIGO * dy / distancia;

    ProyectilEnemigo pe;
    pe.x = enemigo.x;
    pe.y = enemigo.y;
    pe.velX = vx;
    pe.velY = vy;

    proyectilesEnemigos.push_back(pe);
}

void ReaparecerEnemigo() {
    enemigo.x = rand() % (w - 100) + 50;
    enemigo.y = rand() % (h - 100) + 50;

    // Calcula el nuevo radio con un límite mínimo
    double nuevoRadio = 50.0 - (nivel * 2.0); // Ejemplo: nivel 1 = 48, nivel 5 = 40...
    if (nuevoRadio < 15.0) nuevoRadio = 15.0;

    enemigo.radio = nuevoRadio;

    // Aumenta vida segun la dificultad
    enemigo.vidaMax += 20; 
    enemigo.vidaActual = enemigo.vidaMax;
}

//============================================================
// Función para dibujar una textura en una posición específica
void DibujarTextura(GLuint textura, float x, float y, float ancho, float alto) {
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, textura);
    glBegin(GL_QUADS);
    glColor3f(1.0, 1.0, 1.0);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(x, y);
    glTexCoord2f(1.0f, 0.0f); glVertex2f(x + ancho, y);
    glTexCoord2f(1.0f, 1.0f); glVertex2f(x + ancho, y + alto);
    glTexCoord2f(0.0f, 1.0f); glVertex2f(x, y + alto);
    glEnd();
    glDisable(GL_TEXTURE_2D);
}

//============================================================
// Funciones de dibujo

void DibujarTexto(const char* texto, int x, int y) {
    glRasterPos2i(x, y);
    while (*texto) {
        glutBitmapCharacter(GLUT_BITMAP_HELVETICA_18, *texto);
        ++texto;
    }
}

void DibujarCabina() {
    glColor3d(0.6, 0.7, 0.9);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(0, 0);
    for (double r = 0; r < PI * 2; r += 0.1)
        glVertex2d(cos(r), sin(r));
    glVertex2d(1.0, 0.0);
    glEnd();
}

void DibujarCuerpo() {
    glColor3d(0.4, 0.4, 0.4);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(0.0, 0.0);
    glVertex2d(0.0, 70.0);
    glVertex2d(-8, 35.0);
    glVertex2d(-10, -40.0);
    glVertex2d(0.0, -30.0);
    glVertex2d(10, -40.0);
    glVertex2d(8, 35.0);
    glVertex2d(0.0, 70.0);
    glEnd();
}

void DibujarAla() {
    glColor3d(0.7, 0.7, 0.7);
    glBegin(GL_TRIANGLE_FAN);
    glVertex2d(35, 10);
    glVertex2d(0.0, 20.0);
    glVertex2d(0.0, 0.0);
    glVertex2d(35, 4);
    glVertex2d(50.0, 0.0);
    glEnd();
}

void DibujarMotor() {
    glColor3d(0.8, 0.2, 0.2);
    glBegin(GL_QUADS);
    glVertex2d(0, 0);
    glVertex2d(5, 0);
    glVertex2d(5, 15);
    glVertex2d(0, 15);
    glEnd();
}

void DibujarCola() {
    glColor3d(0.4, 0.4, 0.4);
    glBegin(GL_TRIANGLES);
    glVertex2d(0, 0);  
    glVertex2d(-30, -15); 
    glVertex2d(30, -15); 
    glEnd();
}

void DibujarMetralla() {
    glColor3d(0.4, 0.4, 0.4);
    glBegin(GL_QUADS);
    glVertex2d(0, 0);
    glVertex2d(3, 0);
    glVertex2d(3, -8);
    glVertex2d(0, -8);
    glEnd();
}

void DibujarAvion() {
    glPushMatrix();// inicio push1

    // Posiciona y rota el Avion en el modelo
    glTranslated(AvionX, AvionY, 0);
    glRotated(AvionAng, 0, 0, 1);

    //Dibujamos las distintas partes de la nave, aplicando las transformaciones necesarias
    
    //Cabina
    glPushMatrix();
    glScaled(6, 12, 1);
    DibujarCabina();
    glPopMatrix();

    // Motor derecho
    glPushMatrix();
    glTranslated(25, 0, 0);
    DibujarMotor();
    glPopMatrix();

    // Motor izquierdo
    glPushMatrix();
    glTranslated(-25, 0, 0);
    DibujarMotor();
    glPopMatrix();

    //Cuerpo
    DibujarCuerpo();

    //Ala derecha
    glPushMatrix();
    DibujarAla();
    glPopMatrix();

    //Ala izquierda
    glPushMatrix();
    glScaled(-1, 1, 1); //Con este escalamiento logramos reflejar (x = -AnchoAla * x)  
    DibujarAla();
    glPopMatrix();

    // Cola
    glPushMatrix();
    glTranslated(0, -30, 0); // Ajustar la posición de la cola según sea necesario
    DibujarCola();
    glPopMatrix();

    // Metralla derecha
    glPushMatrix();
    glTranslated(45, 10, 0); // Ajustar la posición de la metralla según sea necesario
    DibujarMetralla();
    glPopMatrix();

    // Metralla izquierda
    glPushMatrix();
    glTranslated(-45, 10, 0); // Ajustar la posición de la metralla según sea necesario
    DibujarMetralla();
    glPopMatrix();

    // Luego de aplicar la traslacion (AvionX,AvionY) y la rotacion (AvionAng) inicial 
    // dibuja un punto en la posición 0,0 (es solo para ayuda)
    glColor3f(0.0f, 0.0f, 0.0f);
    glPointSize(5.0);
    glBegin(GL_POINTS);
    glVertex2d(0.0, 0.0);
    glEnd();

    glPopMatrix();// fin push1
}

void DibujarPared() {
    glColor3f(0.9f, 0.9f, 0.9f);
    glLineWidth(5.0);
    glBegin(GL_LINES);
    glVertex2i(325, 400); glVertex2i(325, 200);
    glVertex2i(325, 200); glVertex2i(475, 200);
    glVertex2i(475, 200); glVertex2i(475, 400);
    glEnd();
}

void DibujarProyectiles() {
    glColor3f(1.0, 0.0, 0.0);  
    for (auto& p : proyectiles) {
        glPushMatrix();

        // Ajustar el punto de aparición aquí
        glTranslated(p.x, p.y, 0);
        glRotated(p.angulo, 0, 0, 1);

        glBegin(GL_QUADS);  
        glVertex2f(-2, -2);
        glVertex2f(2, -2);
        glVertex2f(2, 2);
        glVertex2f(-2, 2);
        glEnd();
        glPopMatrix();
    }
}

void DibujarProyectilesEnemigos() {
    glColor3f(1.0, 1.0, 0.0);  // Amarillo
    for (auto& p : proyectilesEnemigos) {
        glPushMatrix();
        glTranslated(p.x, p.y, 0);
        glBegin(GL_QUADS);
        glVertex2f(-3, -3);
        glVertex2f(3, -3);
        glVertex2f(3, 3);
        glVertex2f(-3, 3);
        glEnd();
        glPopMatrix();
    }
}

void DibujarEnemigo(Enemigo& enemigo) {
    if (enemigo.vidaActual <= 0) {
        return; // No dibujar si la vida del enemigo es 0
    }

    // Dibujar el círculo del enemigo
    glColor3f(0.0, 1.0, 0.0);  
    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(enemigo.x, enemigo.y);
    for (int i = 0; i <= 100; ++i) {
        double angulo = 2 * PI * i / 100;
        glVertex2f(enemigo.x + cos(angulo) * enemigo.radio, enemigo.y + sin(angulo) * enemigo.radio);
    }
    glEnd();

    // Dibujar la barra de vida
    double anchoBarra = 60;
    double altoBarra = 10;
    double porcentajeVida = (double)enemigo.vidaActual / enemigo.vidaMax;

    glColor3f(1.0, 0.0, 0.0); 
    glBegin(GL_QUADS);
    glVertex2f(enemigo.x - anchoBarra / 2, enemigo.y + enemigo.radio + 10);
    glVertex2f(enemigo.x - anchoBarra / 2 + anchoBarra * porcentajeVida, enemigo.y + enemigo.radio + 10);
    glVertex2f(enemigo.x - anchoBarra / 2 + anchoBarra * porcentajeVida, enemigo.y + enemigo.radio + 10 + altoBarra);
    glVertex2f(enemigo.x - anchoBarra / 2, enemigo.y + enemigo.radio + 10 + altoBarra);
    glEnd();
}

void DibujarSombraAvion() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); // Habilita transparencia

    glColor4f(0.0, 0.0, 0.0, 0.25); // Negro con 25% de transparencia

    glPushMatrix();
    glTranslated(AvionX + 15, AvionY - 15, 0); // Mueve la sombra un poco más
    glScaled(2.0, 1.0, 1.0); // Hace la sombra más grande y alargada
    glRotated(AvionAng, 0, 0, 1); // Aplica la rotación del avión

    glBegin(GL_TRIANGLE_FAN);
    glVertex2f(0, 0); // Centro del óvalo
    for (int i = 0; i <= 360; i += 10) {
        float rad = i * PI / 180.0;
        glVertex2f(cos(rad) * 25, sin(rad) * 12); // Aumentamos los radios
    }
    glEnd();

    glPopMatrix();
    glDisable(GL_BLEND);
}

bool VerificarColision(Proyectil& p, Enemigo& enemigo) {
    double distX = p.x - enemigo.x;
    double distY = p.y - enemigo.y;
    double distancia = sqrt(distX * distX + distY * distY); 

    return distancia <= enemigo.radio;
}

bool ColisionAvionEnemigo(Enemigo& enemigo) {
    double dx = AvionX - enemigo.x;
    double dy = AvionY - enemigo.y;
    double distancia = sqrt(dx * dx + dy * dy);
    return distancia <= enemigo.radio + 30; // 30 es el radio aproximado del avión
}

void ActualizarEnemigo(Enemigo& enemigo) {
    if (enemigo.vidaActual <= 0) {
        puntaje += 100;

        if (nivel < nivelMaximo) {
            nivel++;
            ReaparecerEnemigo();
        }
        else {
            victoria = true;
            juegoActivo = false;
        }
    }

    // Revisar colisiones con los proyectiles
    for (auto& proyectil : proyectiles) {
        if (VerificarColision(proyectil, enemigo)) {
            enemigo.vidaActual -= 4;  // Disminuir la vida en 4 por cada impacto
            proyectil.x = -9999; // Mover el proyectil fuera de la pantalla al impactar
            proyectil.y = -9999;
        }
    }
}

void ActualizarProyectiles() {
    // Recorremos la lista de proyectiles y eliminamos los que salgan de la pantalla
    for (size_t i = 0; i < proyectiles.size(); i++) {
        // Si el proyectil está fuera de los límites, lo eliminamos
        if (proyectiles[i].x < 0 || proyectiles[i].x > w ||
            proyectiles[i].y < 0 || proyectiles[i].y > h) {
            proyectiles.erase(proyectiles.begin() + i);
            i--;  // Ajustamos el índice después de borrar
        }
    }
}

void LimitarMovimientoAvion() {
    // Limitar la posición del avión dentro de la pantalla
    if (AvionX < 0) AvionX = 0;
    if (AvionX > w) AvionX = w;
    if (AvionY < 0) AvionY = 0;
    if (AvionY > h) AvionY = h;
}

//============================================================
// callbacks

//------------------------------------------------------------

// arma un un nuevo buffer (back) y reemplaza el framebuffer
void Display_cb() {
    // Arma el back-buffer
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT); // rellena con color de fondo y limpia el buffer de profundidad

    // Dibujar la textura de fondo
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaFondo);
    glBegin(GL_QUADS);
    glColor3f(1.0, 1.0, 1.0);
    glTexCoord2f(0.0f, 0.0f); glVertex2f(0.0f, 0.0f);
    glTexCoord2f(2.0f, 0.0f); glVertex2f(w, 0.0f);
    glTexCoord2f(2.0f, 2.0f); glVertex2f(w, h);
    glTexCoord2f(0.0f, 2.0f); glVertex2f(0.0f, h);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    //Dibujar sombra del avion
    DibujarSombraAvion();

    // Dibujar el avión
    DibujarAvion();

    // Dibujar proyectiles
    DibujarProyectiles();

    //Dibujar proyectiles enemigos
    DibujarProyectilesEnemigos();

    // Dibujar el enemigo
    DibujarEnemigo(enemigo);

    // Actualizar el enemigo
    ActualizarEnemigo(enemigo);

    // Dibujar la textura de Jupiter en la esquina superior derecha
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaJupiter);
    DibujarTextura(texturaJupiter, w - 150, h - 150, 150, 150);
    glDisable(GL_TEXTURE_2D);

    // Dibujar la textura de Galaxia en la esquina inferior izquierda
    glEnable(GL_TEXTURE_2D);
    glBindTexture(GL_TEXTURE_2D, texturaGalaxia);
    DibujarTextura(texturaGalaxia, 0, 0, 150, 150);
    glDisable(GL_TEXTURE_2D);

    // Dibujar instrucciones
    glColor3f(1.0, 1.0, 1.0); // Color del texto: blanco
    DibujarTexto("Teclas a utilizar:", 10, 580);
    DibujarTexto("w: avanza", 10, 560);
    DibujarTexto("s: retrocede", 10, 540);
    DibujarTexto("d: gira en sentido horario", 10, 520);
    DibujarTexto("a: gira en sentido antihorario", 10, 500);
    DibujarTexto("click izq: disparar", 10, 480);
    DibujarTexto("t: turbo", 10, 460);

    // Mostrar posición y rotación del avión
    glColor3f(1.0, 1.0, 1.0); // Color del texto: blanco

    char texto[50];
    sprintf(texto, " ");
    DibujarTexto(texto, 10, 440); // Muestra la posición

    sprintf(texto, "Posicion: (%.1f, %.1f)", AvionX, AvionY);
    DibujarTexto(texto, 10, 420); // Muestra la posición

    sprintf(texto, "Rotacion: %.1f°", AvionAng);
    DibujarTexto(texto, 10, 400); // Muestra la rotación

    sprintf(texto, "Vidas: %d", vidasAvion);
    glColor3f(0.0, 1.0, 0.0);
    DibujarTexto(texto, 10, 380);

    sprintf(texto, "Puntaje: %d", puntaje);
    glColor3f(1.0, 1.0, 0.0);
    DibujarTexto(texto, 10, 360);

    sprintf(texto, "Nivel: %d", nivel);
    glColor3f(1.0, 0.0, 1.0);
    DibujarTexto(texto, 10, 340);

    if (turboActivo) {
        sprintf(texto, "TURBO ACTIVO (%.1f seg)", framesTurboRestantes / 60.0);
        glColor3f(1.0, 1.0, 0.0); // Amarillo
    }
    else if (framesCooldownRestante > 0) {
        sprintf(texto, "Turbo disponible en %.1f seg", framesCooldownRestante / 60.0);
        glColor3f(1.0, 0.5, 0.0); // Naranja
    }
    else {
        sprintf(texto, "Turbo disponible: SI");
        glColor3f(0.0, 1.0, 0.0); // Verde
    }
    DibujarTexto(texto, 10, 320);

    if (!juegoActivo) {
        if (victoria) {
            glColor3f(0.0, 1.0, 0.0); // Verde
            DibujarTexto("¡Felicitaciones! ¡Lograste la victoria!", w / 2 - 140, h / 2);
            DibujarTexto("Presiona ESC para salir", w / 2 - 100, h / 2 - 30);
        }
        else {
            glColor3f(1.0, 0.0, 0.0); // Rojo
            DibujarTexto("GAME OVER - Presiona ESC para salir", w / 2 - 140, h / 2);
        }
    }

    glutSwapBuffers(); // lo manda al monitor

    // Chequea errores
    int errornum = glGetError();
    while (errornum != GL_NO_ERROR) {
        if (cl_info) {
            if (errornum == GL_INVALID_ENUM)
                cout << "GL_INVALID_ENUM" << endl;
            else if (errornum == GL_INVALID_VALUE)
                cout << "GL_INVALID_VALUE" << endl;
            else if (errornum == GL_INVALID_OPERATION)
                cout << "GL_INVALID_OPERATION" << endl;
            else if (errornum == GL_STACK_OVERFLOW)
                cout << "GL_STACK_OVERFLOW" << endl;
            else if (errornum == GL_STACK_UNDERFLOW)
                cout << "GL_STACK_UNDERFLOW" << endl;
            else if (errornum == GL_OUT_OF_MEMORY)
                cout << "GL_OUT_OF_MEMORY" << endl;
        }
        errornum = glGetError();
    }
}

void Reshape_cb(int Width, int Height) {
    glViewport(0, 0, Width, Height);
    glMatrixMode(GL_PROJECTION); glLoadIdentity();
    gluOrtho2D(0, Width, 0, Height);
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();
    w = Width; h = Height;
    glutPostRedisplay(); // actualiza la pantalla con la nueva configuración
}
//------------------------------------------------------------
// Teclado

// Maneja pulsaciones del teclado (ASCII keys)
// x,y posición del mouse cuando se teclea (aquí no importan)
void Keyboard_cb(unsigned char key, int x, int y) {
    teclas[key] = true;
    double ang = AvionAng * PI / 180.0;
    double velocidadActual = turboActivo ? VELOCIDAD_TURBO : VELOCIDAD_BASE;

    switch (key) {
    case 'w':
    case 'W':
        AvionX -= velocidadActual * sin(ang);
        AvionY += velocidadActual * cos(ang);
        glutPostRedisplay();
        break;
    case 's':
    case 'S':
        AvionX += velocidadActual * sin(ang);
        AvionY -= velocidadActual * cos(ang);
        glutPostRedisplay();
        break;
    case 'a':
    case 'A':
        AvionAng += 2;
        glutPostRedisplay();
        break;
    case 'd':
    case 'D':
        AvionAng -= 2;
        glutPostRedisplay();
        break;
    case 27:
        exit(EXIT_SUCCESS);
        break;
    }

    // Activar turbo si se presiona T
    if ((key == 't' || key == 'T') && !turboActivo && framesCooldownRestante == 0) {
        turboActivo = true;
        framesTurboRestantes = DURACION_TURBO;
    }
}

void Keyboard_up_cb(unsigned char key, int x, int y) {
    teclas[key] = false; // Marca la tecla como no presionada
}

// Special keys (non-ASCII)
// teclas de función, flechas, page up/dn, home/end, insert
void Special_cb(int key, int xm = 0, int ym = 0) {
    if (key == GLUT_KEY_F4 && glutGetModifiers() == GLUT_ACTIVE_ALT)
        exit(EXIT_SUCCESS);

}

void Mouse_cb(int button, int state, int x, int y) {
    if (button == GLUT_LEFT_BUTTON && state == GLUT_DOWN) {
        CrearProyectiles();
    }
}
//------------------------------------------------------------

// Función Idle para controlar el tiempo
void Idle_cb() {
    AvionY -= GRAVEDAD; // Simulación de la gravedad
    double velocidadActual = turboActivo ? VELOCIDAD_TURBO : VELOCIDAD_BASE;

    // Control del turbo
    if (turboActivo) {
        framesTurboRestantes--;
        if (framesTurboRestantes <= 0) {
            turboActivo = false;
            framesCooldownRestante = ENFRIAMIENTO_TURBO;
        }
    }
    else {
        if (framesCooldownRestante > 0) {
            framesCooldownRestante--;
        }
    }

    // Movimiento en función de teclas presionadas
    double ang = AvionAng * PI / 180.0;

    if (teclas['w'] || teclas['W']) {
        AvionX -= velocidadActual * sin(ang);
        AvionY += velocidadActual * cos(ang);

    }
    if (teclas['s'] || teclas['S']) {
        AvionX += velocidadActual * sin(ang);
        AvionY -= velocidadActual * cos(ang);

    }
    if (teclas['a'] || teclas['A']) {
        AvionAng += 2;
    }
    if (teclas['d'] || teclas['D']) {
        AvionAng -= 2;
    }

    // Actualizar la posición de los proyectiles
    for (auto& p : proyectiles) {
        p.x += p.velX;
        p.y += p.velY;
    }

    // Redibuja la ventana
    ActualizarProyectiles();
    LimitarMovimientoAvion();

    if (juegoActivo && enemigo.vidaActual > 0 && ColisionAvionEnemigo(enemigo)) {
        vidasAvion--;
        AvionX = 400;  // Reiniciar posición
        AvionY = 300;

        if (vidasAvion <= 0) {
            juegoActivo = false;
        }
    }

    // Mover proyectiles enemigos
    for (size_t i = 0; i < proyectilesEnemigos.size(); i++) {
        proyectilesEnemigos[i].x += proyectilesEnemigos[i].velX;
        proyectilesEnemigos[i].y += proyectilesEnemigos[i].velY;

        // Colisión con el avión
        double dx = proyectilesEnemigos[i].x - AvionX;
        double dy = proyectilesEnemigos[i].y - AvionY;
        double distancia = sqrt(dx * dx + dy * dy);

        if (distancia <= 20 && juegoActivo) {
            vidasAvion--;
            proyectilesEnemigos.erase(proyectilesEnemigos.begin() + i);
            i--;

            if (vidasAvion <= 0) {
                juegoActivo = false;
            }

            continue;
        }

        // Eliminar si sale de pantalla
        if (proyectilesEnemigos[i].x < 0 || proyectilesEnemigos[i].x > w ||
            proyectilesEnemigos[i].y < 0 || proyectilesEnemigos[i].y > h) {
            proyectilesEnemigos.erase(proyectilesEnemigos.begin() + i);
            i--;
        }
    }

    contadorDisparo++;
    if (contadorDisparo >= intervaloDisparo) {
        DispararDesdeEnemigo();
        contadorDisparo = 0;
    }

    glutPostRedisplay();
}

//------------------------------------------------------------
void inicializa() {
    // GLUT
    glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH); // pide color RGB, double buffering y depth buffer
    glutInitWindowSize(w, h); glutInitWindowPosition(10, 10);
    glutCreateWindow("Ejemplo Avion"); // crea el main window

    // declara los callbacks, los que (aún) no se usan (aún) no se declaran
    glutDisplayFunc(Display_cb);
    glutReshapeFunc(Reshape_cb);
    glutKeyboardFunc(Keyboard_cb); // Tecla presionada
    glutKeyboardUpFunc(Keyboard_up_cb); // Tecla liberada
    glutSpecialFunc(Special_cb);
    glutIdleFunc(Idle_cb); // Registra el callback Idle
    glutMouseFunc(Mouse_cb); // Registrar el callback del ratón

    // OpenGL
    glClearColor(0.01f, 0.01f, 0.01f, 1.f); // color de fondo
    glMatrixMode(GL_MODELVIEW); glLoadIdentity();

    // Cargar texturas
    texturaFondo = CargarTexturaPPM("fondo.ppm");
    texturaJupiter = CargarTexturaPPM("jupiter.ppm");
    texturaGalaxia = CargarTexturaPPM("galaxia.ppm");

}

//------------------------------------------------------------
// main
int main(int argc, char** argv) {
    glutInit(&argc, argv); // inicialización interna de GLUT
    inicializa(); // define el estado inicial de GLUT y OpenGL
    glutMainLoop(); // entra en loop de reconocimiento de eventos
    return 0; // nunca se llega acá, es sólo para evitar un warning
}