#include <windows.h>

#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/glut.h>

#include "Model3D.h"

Model3D * model = new Model3D();
float rotate = 0.0f;

void display(void) {
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	// Clear The Screen And The Depth Buffer
	glLoadIdentity();				// Reset MV Matrix

	gluLookAt(
		0.0f, 30.0f, 30.0f,		// Camera position
		0.0f, 0.0f, 0.0f,		// Camera look at
		0.0f, 1.0f, 0.0f		// Camera vector up
	);

	glRotatef(rotate, 0.0f, 1.0f, 0.0f);
	model->render();

	rotate += 0.1f;

	glFlush();
	glutSwapBuffers();
	glutPostRedisplay();
}

int main(int argc, char **argv)
{
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGBA | GLUT_DEPTH | GLUT_DOUBLE);
	glutInitWindowSize(1024, 768);
	glutInitWindowPosition(100, 100);
	glutCreateWindow("Loader OBJ");
	glutDisplayFunc(display);

	glViewport(0, 0, 1024, 768);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(45.0f, (GLfloat)1024 / (GLfloat)768, 0.1f, 100.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glEnable(GL_TEXTURE_2D);
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClearDepth(1.0f);
	glEnable(GL_DEPTH_TEST);

	glEnable(GL_LIGHTING);
	glEnable(GL_LIGHT0);

	if (!model->loadModel("star.obj"))
		return 1;

	if (!model->loadTexture())
		return 1;

	glutMainLoop();

	return 0;
}

