#include "particle.h"
#include "GLUT/glut.h"
#include "scene.h"

Particle::Particle(Vector3d position) {
	this->position = position;
	this->rho0 = Scene::rho0;
	this->mass = Scene::volume * rho0;
	this->color = Vector3d(0.2,0.2,1);
}

void Particle::draw() {
    //glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glPushMatrix();
		glColor3d(color.x(), color.y(), color.z());
        glTranslated(position.x(), position.y(), position.z());
    	glutSolidSphere(Scene::d*0.4, 8, 2);
    glPopMatrix(); 
}