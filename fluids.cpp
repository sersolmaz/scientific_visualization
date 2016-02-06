// Usage: Drag with the mouse to add smoke to the fluid. This will also move a "rotor" that disturbs
//        the velocity field at the mouse location. Press the indicated keys to change options
//--------------------------------------------------------------------------------------------------


#include <stdio.h>              //for printing the help text
#include <math.h>               //for various math functions
#include <GLUT/glut.h>          //the GLUT graphics library
#include "visualization.cpp"		//for visualization
#include "simulation.h"
#include "simulation.cpp"
//------ INTERACTION CODE STARTS HERE -----------------------------------------------------------------

//display: Handle window redrawing events. Simply delegates to visualize().
void display(void)
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	visualize();
	glFlush();
	glutSwapBuffers();
}

//reshape: Handle window resizing (reshaping) events
void reshape(int w, int h)
{
 	glViewport(0.0f, 0.0f, (GLfloat)w, (GLfloat)h);
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0, (GLdouble)w, 0.0, (GLdouble)h);
	winWidth = w; winHeight = h;
}

//keyboard: Handle key presses
void keyboard(unsigned char key, int x, int y)
{
	switch (key)
	{
      case 't': simulation.set_dt(simulation.get_dt() - 0.001); break;
      case 'T': simulation.set_dt(simulation.get_dt() + 0.001); break;
	  case 'c': color_dir = 1 - color_dir; break;
	  case 'S': vec_scale *= 1.2; break;
	  case 's': vec_scale *= 0.8; break;
      case 'V': simulation.set_visc(simulation.get_visc()*5); break;
      case 'v': simulation.set_visc(simulation.get_visc()*0.2); break;
	  case 'x': draw_smoke = 1 - draw_smoke;
		    if (draw_smoke==0) draw_vecs = 1; break;
	  case 'y': draw_vecs = 1 - draw_vecs;
		    if (draw_vecs==0) draw_smoke = 1; break;
	  case 'm': scalar_col++; if (scalar_col>COLOR_BANDS) scalar_col=COLOR_BLACKWHITE; break;
      case 'a': simulation.set_frozen(1-simulation.get_frozen()); break;
	  case 'q': exit(0);
	}
}



// drag: When the user drags with the mouse, add a force that corresponds to the direction of the mouse
//       cursor movement. Also inject some new matter into the field at the mouse location.
void drag(int mx, int my)
{
	int xi,yi,X,Y; double  dx, dy, len;
	static int lmx=0,lmy=0;				//remembers last mouse location

	// Compute the array index that corresponds to the cursor location
    xi = (int)simulation.clamp((double)(simulation.DIM + 1) * ((double)mx / (double)winWidth));
    yi = (int)simulation.clamp((double)(simulation.DIM + 1) * ((double)(winHeight - my) / (double)winHeight));

	X = xi; Y = yi;

    if (X > (simulation.DIM - 1))  X = simulation.DIM - 1; if (Y > (simulation.DIM - 1))  Y = simulation.DIM - 1;
	if (X < 0) X = 0; if (Y < 0) Y = 0;

	// Add force at the cursor location
	my = winHeight - my;
	dx = mx - lmx; dy = my - lmy;
	len = sqrt(dx * dx + dy * dy);
	if (len != 0.0) {  dx *= 0.1 / len; dy *= 0.1 / len; }
    simulation.get_fx()[Y * simulation.DIM + X] += dx;
    simulation.get_fy()[Y * simulation.DIM + X] += dy;
    simulation.get_rho()[Y * simulation.DIM + X] = 10.0f;
	lmx = mx; lmy = my;
}

//do_one_simulation_step: Do one complete cycle of the simulation:
//      - set_forces:
//      - solve:            read forces from the user
//      - diffuse_matter:   compute a new set of velocities
//      - gluPostRedisplay: draw a new visualization frame
void do_one_simulation_step(void)
{
    if (!simulation.get_frozen())
    {
        simulation.set_forces();
        simulation.solve(simulation.DIM, simulation.get_vx(), simulation.get_vy(), simulation.get_vx0(),
                         simulation.get_vy0(), simulation.get_visc(), simulation.get_dt());
        // Note to self: * in *simulation.get_vx() because simulation.get_vx() returns 'double *' and diffuse_matter needs 'double'
        simulation.diffuse_matter(simulation.DIM, simulation.get_vx(), simulation.get_vy(),
                                  simulation.get_rho(), simulation.get_rho0(), simulation.get_dt());
        glutPostRedisplay();
    }
}



//main: The main program
int main(int argc, char **argv)
{
	printf("Fluid Flow Simulation and Visualization\n");
	printf("=======================================\n");
	printf("Click and drag the mouse to steer the flow!\n");
	printf("T/t:   increase/decrease simulation timestep\n");
	printf("S/s:   increase/decrease hedgehog scaling\n");
	printf("c:     toggle direction coloring on/off\n");
	printf("V/v:   increase decrease fluid viscosity\n");
	printf("x:     toggle drawing matter on/off\n");
	printf("y:     toggle drawing hedgehogs on/off\n");
	printf("m:     toggle thru scalar coloring\n");
	printf("a:     toggle the animation on/off\n");
	printf("q:     quit\n\n");
	glutInit(&argc, argv);
	glutInitDisplayMode(GLUT_RGB | GLUT_DOUBLE | GLUT_DEPTH);
	glutInitWindowSize(500,500);
	glutCreateWindow("Real-time smoke simulation and visualization");
	glutDisplayFunc(display);
	glutReshapeFunc(reshape);
    glutIdleFunc(do_one_simulation_step);
	glutKeyboardFunc(keyboard);
	glutMotionFunc(drag);

    simulation.init_simulation(simulation.DIM);	//initialize the simulation data structures
	glutMainLoop();			//calls do_one_simulation_step, keyboard, display, drag, reshape
	return 0;
}

