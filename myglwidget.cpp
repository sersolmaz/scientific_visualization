// myglwidget.cpp

#include <QtWidgets>
#include <QtOpenGL>
#include <GLUT/glut.h>          //the GLUT graphics library
#include "myglwidget.h"
#include "visualization.cpp"
#include <simulation.cpp>              //the numerical simulation FFTW library
#include <cmath>
#include "vector.cpp"
#include "grid.h"
#include <queue>

MyGLWidget::MyGLWidget(QWidget *parent)
{
    //--- VISUALIZATION PARAMETERS ---------------------------------------------------------------------
    hedgehog_scale = 2000;			//scaling of hedgehogs
    arrow_scale = 1000;			//scaling of arrows
    cone_scale = 1000;          //scaling of cones
    draw_smoke = true;           //draw the smoke or not
    draw_vecs = true;            //draw the vector field or not
    scalar_col = 0;           //method for scalar coloring
    scale_color = false;    // if true, the lowest current value in the screen is the lowest in the color map, same for highest
    DIM = 50;
    // should change to have a color map class that has color clamp values
    color_clamp_min_matter = 0.0;        // The lower bound value to clamp color map at
    color_clamp_max_matter = 1.0;        // The higher bound value to clamp color map at
    color_clamp_min_glyph = 0.0;        // The lower bound value to clamp color map at
    color_clamp_max_glyph = 1.0;        // The higher bound value to clamp color map at
    velocity_color = 1;
    number_of_glyphs = DIM*2;
    force_field_color = 1;
    grid_scale = 1;             // when drawing the grid, the size per cell is grid_scale * cell size, so with 50x50 grid with grid_scale = 10, 5 cells will be drawn
    color_bands = 256;
    hue = 1;
    draw_grid = false;
    draw_slices = false;
    glyphs = "hedgehogs";
    dataset = "fluid velocity magnitude";
    gradient = false;
    draw_streamline = false;
    simulation.init_simulation(DIM);
    QTimer *timer = new QTimer;
    timer->start(1);
    QObject::connect(timer,SIGNAL(timeout()),this,SLOT(do_one_simulation_step()));
}

MyGLWidget::~MyGLWidget()
{
}

void MyGLWidget::initializeGL()
{
    qglClearColor(Qt::black);
}

void MyGLWidget::paintGL() //glutDisplayFunc(display);
{
    glEnable(GL_DEPTH_TEST);
    glEnable(GL_COLOR_TABLE);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glMatrixMode(GL_MODELVIEW);
    //glLoadIdentity();
    drawBar();
    cell_width = ceil((fftw_real)windowWidth / (fftw_real)(DIM));   // Grid cell width
    cell_height = ceil((fftw_real)windowHeight / (fftw_real)(DIM));  // Grid cell heigh

    if (draw_grid){
        drawGridLines(DIM);
    }
    if(draw_slices){
        drawSlices(2);
    }
    if (draw_streamline){
        drawStreamline();
    }
    if (draw_vecs)
    {
        if(dataset == "fluid velocity magnitude"){
            drawVelocity(simulation.get_vx(), simulation.get_vy());
        }
        else if (dataset == "force field magnitude"){
            drawForcefield(simulation.get_fx(), simulation.get_fy());
        }
    }
    if (gradient){
        drawGradient();
    }
    if (draw_smoke)
    {
        drawSmoke();
    }

    //OGL_Draw_Text();
    glFlush();
}

void MyGLWidget::resizeGL(int width, int height)
{
    // removing below had no effect on what is drawn, so better to not use to lower complexity
    //glViewport(0.0f, 0.0f, (GLfloat)width, (GLfloat)height);
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (GLdouble)width, 0.0, (GLdouble)height);
    windowWidth = width; windowHeight = height;
}

void MyGLWidget::mousePressEvent(QMouseEvent *event)
{
    lastPos = event->pos();
}

void MyGLWidget::mouseMoveEvent(QMouseEvent *event)
{
    int mx = event->x();// - lastposition gets calculated in drag(), could save a step by using lastPos.x/y but leaving it like this is safer
    int my = event->y();
    simulation.drag(mx,my, DIM, windowWidth, windowHeight);  // Works for Freerk when using external display
    //simulation.drag(mx,my, DIM, windowWidth/2, windowHeight/2); // Works for Niek
}

void MyGLWidget::drawGradient()
{
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++)
        {
            float left_rho =  simulation.get_rho()[((j-1) * DIM) + i];
            float up_rho =  simulation.get_rho()[(j * DIM) + (i-1)];
            float below_rho =  simulation.get_rho()[(j * DIM) + (i+1)];
            float right_rho =  simulation.get_rho()[((j+1) * DIM) + (i)];
            float x = left_rho - right_rho;
            float y = up_rho - below_rho;
            Vector v = Vector(y, x);
            if (v.length() > 0.1){
                drawArrow(v, i, j, v.length(), 2);
            }
        }
}



void MyGLWidget::drawVelocity(fftw_real *vx, fftw_real *vy)
{
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++)
        {
            int idx = (j * DIM) + i;
            if (glyphs == "hedgehogs"){
                if (i % (100/number_of_glyphs) == 0 && j % (100/number_of_glyphs)  == 0){
                    glBegin(GL_LINES);				//draw velocities
                    direction_to_color(vx[idx], vy[idx], velocity_color, color_bands, color_clamp_min_glyph, color_clamp_max_glyph, hue);
                    glVertex2f((fftw_real)i * cell_width, (fftw_real)j * cell_height);
                    glVertex2f((fftw_real)i * cell_width + hedgehog_scale * vx[idx], (fftw_real)j * cell_height + hedgehog_scale * vy[idx]);
                    glEnd();
                }
            }
            else if (glyphs == "arrows"){
                if (i % (100/number_of_glyphs) == 0 && j % (100/number_of_glyphs)  == 0){
                    int idx = (j * DIM) + i;
                    Vector vector = Vector((fftw_real)i * cell_width, //x1
                                           (fftw_real)j * cell_height, //y1
                                           ((fftw_real)i * cell_width) + arrow_scale * vx[idx], //x2
                                           ((fftw_real)j * cell_height) + arrow_scale * vy[idx]);//y2

                    drawArrow(vector, i, j, vector.length()/15, 10);
                }
            }
            else if (glyphs == "cones"){
                //
                if (i % (200/number_of_glyphs) == 0 && j % (200/number_of_glyphs)  == 0){
                    int idx = (j * DIM) + i;
                    Vector vector = Vector((fftw_real)i * cell_width, //x1
                                           (fftw_real)j * cell_height, //y1
                                           ((fftw_real)i * cell_width) + cone_scale * vx[idx], //x2
                                           ((fftw_real)j * cell_height) + cone_scale * vy[idx]);//y2

                    drawCone(vector, i, j, vector.length()/15, 10);
                }
            }
        }
}


void MyGLWidget::drawForcefield(fftw_real *fx, fftw_real *fy)
{
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++)
        {
            int idx = (j * DIM) + i;
            if (glyphs == "hedgehogs"){
                if (i % (100/number_of_glyphs) == 0 && j % (100/number_of_glyphs)  == 0){
                    glBegin(GL_LINES);				//draw velocities
                    direction_to_color(fx[idx], fy[idx], velocity_color, color_bands, color_clamp_min_glyph, color_clamp_max_glyph, hue);
                    glVertex2f((fftw_real)i * cell_width, (fftw_real)j * cell_height);
                    glVertex2f((fftw_real)i * cell_width + hedgehog_scale * fx[idx], (fftw_real)j * cell_height + hedgehog_scale * fy[idx]);
                    glEnd();
                }
            }
            else if (glyphs == "arrows"){
                if (i % (100/number_of_glyphs) == 0 && j % (100/number_of_glyphs)  == 0){
                    int idx = (j * DIM) + i;
                    Vector vector = Vector((fftw_real)i * cell_width, //x1
                                           (fftw_real)j * cell_height, //y1
                                           ((fftw_real)i * cell_width) + arrow_scale * fx[idx], //x2
                                           ((fftw_real)j * cell_height) + arrow_scale * fy[idx]);//y2

                    drawArrow(vector, i, j, vector.length()/15, 10);
                }
            }
            else if (glyphs == "cones"){
                //
                if (i % (200/number_of_glyphs) == 0 && j % (200/number_of_glyphs)  == 0){
                    int idx = (j * DIM) + i;
                    Vector vector = Vector((fftw_real)i * cell_width, //x1
                                           (fftw_real)j * cell_height, //y1
                                           ((fftw_real)i * cell_width) + cone_scale * fx[idx], //x2
                                           ((fftw_real)j * cell_height) + cone_scale * fy[idx]);//y2

                    drawCone(vector, i, j, vector.length()/15, 10);
                }
            }
        }
}


void MyGLWidget::drawArrow(Vector vector, int i, int j, float vy, int scaling_factor){
    // draw an arrow the size of a cell, scale according to vector length
    float angle = vector.normalize().direction2angle();

    set_colormap(vy, velocity_color, color_clamp_min_glyph, color_clamp_max_glyph, color_bands, hue);
    glPushMatrix();
    glTranslatef(cell_width*i,cell_height*j, 0);
    glRotated(angle,0,0,1);
    glScaled(log(vector.length()/scaling_factor+1),log(vector.length()/(scaling_factor/2)+1),0);
    //glScaled(log(vector.length()/2+1),log(vector.length()*5+1),0);
    glBegin(GL_TRIANGLES);
    // arrow base size of 2/20th of cell width
    float size_right = (cell_width/20)*11.0;
    float size_left = (cell_width/20)*9.0;
    float half_cell_height = cell_height/2.0;
    // arrow head, whole cell width, 1/3 of cell heigth
    glVertex2f(0, half_cell_height);            //base1
    glVertex2f(cell_width/2, cell_height);       //tip
    glVertex2f(cell_width, half_cell_height);    //base2
    // arrow tail (made up of 2 triangles)
    glScaled(log(vector.length()+1),log(vector.length()+1),0);
    glVertex2f(size_right, half_cell_height);
    glVertex2f(size_left, 0);
    glVertex2f(size_left, half_cell_height);
    glVertex2f(size_right, 0);
    glVertex2f(size_right, half_cell_height);
    glVertex2f(size_left, 0);

    glEnd();
    glPopMatrix(); // now it's at normal scale again
    glLoadIdentity(); // needed to stop the rotating, otherwise rotates the entire drawing
}


void MyGLWidget::drawCone(Vector vector, int i, int j, float vy, int scaling_factor){
    // draw
    float angle = vector.normalize().direction2angle();

    set_colormap(vy, velocity_color, color_clamp_min_glyph, color_clamp_max_glyph, color_bands);
    glPushMatrix();
    glTranslatef(cell_width*i,cell_height*j, 0);
    glRotated(angle,0,0,1);
    glScaled(log(vector.length()/scaling_factor+1),log(vector.length()/(scaling_factor/2)+1),0);

    // draw the upper part of the cone
    glBegin(GL_TRIANGLE_FAN);
    glVertex3f(0, 0, 0);
    // Smooth shading
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glShadeModel(GL_SMOOTH);
    glHint(GL_LINE_SMOOTH_HINT, GL_NICEST);
    //glEnable (GL_LIGHTING);
    float radius = cell_height/3; // calculate radius
    // foreach degree angle draw circle + arrow point
    for (int angle = 1; angle <= 360; angle++) {
        glColor4f(0,0,0,0.5-(0.5/angle)); // colors(R, G, B, alpha)
        glVertex2f(cell_width/2, cell_height); // draw cone point/tip
        glVertex2f(sin(angle) * radius, cos(angle) * radius); // draw cone base (circle)
    }

    glEnd();
    glPopMatrix(); // now it's at normal scale again
    glLoadIdentity(); // needed to stop the rotating, otherwise rotates the entire drawing
}


void MyGLWidget::drawSlices(int n){
    // n = number of slices (timepoints) to draw
    // use std::queue instead of std::list because it forces FIFO
    std::deque<Grid> grid_timepoints;
    for(int y = 0; y < n; y++){
        do_one_simulation_step(false);
        Grid grid = Grid(DIM);
        for (int i = 0; i < DIM; i++){
            for (int j = 0; j < DIM; j++){
                int idx = (j * DIM) + i;
                grid.addElementToGrid(simulation.get_vx()[idx], simulation.get_vy()[idx], idx);
            }
        }
        grid_timepoints.push_front(grid);
    }
    Grid popped_grid = grid_timepoints.front();
    grid_timepoints.pop_back();
    drawVelocity(popped_grid.vx, popped_grid.vy);
    //updateGL();
}

void MyGLWidget::drawStreamline()
{

    //drawStreamline(25,25);
    for (int i = 0; i < DIM; i++)
        for (int j = 0; j < DIM; j++)
        {

            if (i % 5 == 0 && j % 5 == 0){
                //if ( i ==20 && j==20){
                int idx_1 = (j * DIM) + i;
                int idx_2 = (j * DIM) + i+1;
                int idx_3 = (j+1 * DIM) + i;
                int idx_4 = (j+1 * DIM) + i+1;
                float dt = cell_width/3;
                float max_size = cell_width*3;
                float vertex_x = (fftw_real)i * cell_width;
                float vertex_y = (fftw_real)j * cell_height;
                float start_x = vertex_x + 0.01; // to make sure we are in the cell and not on  the vertex
                float start_y = vertex_y + 0.01; // to make sure we are in the cell and not on  the vertex
                for (int y = 0; y < max_size; y+=dt){
                    Vector vector1 = Vector((fftw_real)i * cell_width, //x1
                                            (fftw_real)j * cell_height, //y1
                                            ((fftw_real)i * cell_width) + simulation.get_vx()[idx_1], //x2
                                            ((fftw_real)j * cell_height) + simulation.get_vy()[idx_1]);//y2
                    Vector vector2 = Vector((fftw_real)i * cell_width, //x1
                                            (fftw_real)j * cell_height, //y1
                                            ((fftw_real)i * cell_width) + simulation.get_vx()[idx_2], //x2
                                            ((fftw_real)j * cell_height) + simulation.get_vy()[idx_2]);//y2
                    Vector vector3 = Vector((fftw_real)i * cell_width, //x1
                                            (fftw_real)j * cell_height, //y1
                                            ((fftw_real)i * cell_width) + simulation.get_vx()[idx_3], //x2
                                            ((fftw_real)j * cell_height) + simulation.get_vy()[idx_3]);//y2
                    Vector vector4 = Vector((fftw_real)i * cell_width, //x1
                                            (fftw_real)j * cell_height, //y1
                                            ((fftw_real)i * cell_width) + simulation.get_vx()[idx_4], //x2
                                            ((fftw_real)j * cell_height) + simulation.get_vy()[idx_4]);//y2
                    Vector interpolated_vector = Vector(0,0);

                    interpolated_vector.interpolate(vector1, vector2, vector3, vector4, start_x,start_y, vertex_x, vertex_y, cell_width);
                    // if outside the grid, stop the stream line
                    //if(interpolated_vector.X > DIM*cell_width || interpolated_vector.Y > DIM*cell_height || interpolated_vector.X <0 || interpolated_vector.Y <0 ){
                    //    return;
                    //}
                    float length  = interpolated_vector.length();
                    if(length>0){
                        interpolated_vector.X = interpolated_vector.X / length;
                        interpolated_vector.Y = interpolated_vector.Y / length;
                        interpolated_vector.X += interpolated_vector.X * dt;
                        interpolated_vector.Y += interpolated_vector.Y * dt;

                        //DRAW
                        glBegin(GL_LINES);				//draw
                        qglColor(Qt::white);
                        glVertex2f(start_x, start_y);
                        glVertex2f(interpolated_vector.X+start_x, interpolated_vector.Y+start_y);
                        glEnd();
                        start_x = interpolated_vector.X+start_x;
                        start_y = interpolated_vector.Y+start_y;
                        int x_axis = floor(start_x/cell_width);
                        int y_axis = floor(start_y/cell_height);
                        idx_1 = (y_axis * DIM) + x_axis;
                        idx_2 = (y_axis * DIM) + x_axis+1;
                        idx_3 = (y_axis+1 * DIM) + x_axis;
                        idx_4 = (y_axis+1 * DIM) + x_axis+1;
                        vertex_x = (fftw_real)i * cell_width;
                        vertex_y = (fftw_real)j * cell_height;
                    }
                }
            }
        }
}

void MyGLWidget::drawSmoke(){
    int  i, j, idx0, idx1, idx2, idx3; double px0,py0,px1,py1,px2,py2,px3,py3;
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    glBegin(GL_TRIANGLES);
    for (i = 0; i < DIM; i++)			//draw smoke
    {
        for (j = 0; j < DIM; j++)
        {
            px0  = floor((fftw_real)i * cell_width);
            py0  = floor((fftw_real)j * cell_height);
            idx0 = (j * DIM) + i;

            px1  = floor((fftw_real)i * cell_width);
            py1  = floor((fftw_real)(j + 1) * cell_height);
            idx1 = ((j + 1) * DIM) + i;

            px2  = floor((fftw_real)(i + 1) * cell_width);
            py2  = floor((fftw_real)(j + 1) * cell_height);
            idx2 = ((j + 1) * DIM) + (i + 1);

            px3  = floor((fftw_real)(i + 1) * cell_width);
            py3  = floor((fftw_real)j * cell_height);
            idx3 = (j * DIM) + (i + 1);
            set_colormap(simulation.get_rho()[idx0], scalar_col, color_clamp_min_matter, color_clamp_max_matter, color_bands, hue);
            glVertex2f(px0, py0);
            set_colormap(simulation.get_rho()[idx1], scalar_col, color_clamp_min_matter, color_clamp_max_matter, color_bands, hue);
            glVertex2f(px1, py1);
            set_colormap(simulation.get_rho()[idx2], scalar_col, color_clamp_min_matter, color_clamp_max_matter, color_bands, hue);
            glVertex2f(px2, py2);

            set_colormap(simulation.get_rho()[idx0], scalar_col, color_clamp_min_matter, color_clamp_max_matter, color_bands, hue);
            glVertex2f(px0, py0);
            set_colormap(simulation.get_rho()[idx2], scalar_col, color_clamp_min_matter, color_clamp_max_matter, color_bands, hue);
            glVertex2f(px2, py2);
            set_colormap(simulation.get_rho()[idx3], scalar_col, color_clamp_min_matter, color_clamp_max_matter, color_bands, hue);
            glVertex2f(px3, py3);
        }
    }
    glEnd();
}

void MyGLWidget::do_one_simulation_step(bool update)
{
    if (!simulation.get_frozen())
    {
        simulation.set_forces(DIM);
        simulation.solve(DIM, simulation.get_vx(), simulation.get_vy(), simulation.get_vx0(),
                         simulation.get_vy0(), simulation.get_visc(), simulation.get_dt());
        // Note to self: * in *simulation.get_vx() because simulation.get_vx() returns 'double *' and diffuse_matter needs 'double'
        simulation.diffuse_matter(DIM, simulation.get_vx(), simulation.get_vy(),
                                  simulation.get_rho(), simulation.get_rho0(), simulation.get_dt());
        if(update){
            updateGL();
        }
    }
}

void MyGLWidget::do_one_simulation_step()
{
    do_one_simulation_step(true);
}

void MyGLWidget::showAnimation(bool new_frozen)
{
    // ! because if the checkbox = true, frozen should be set to false
    simulation.set_frozen(!new_frozen);
}

void MyGLWidget::drawMatter(bool new_draw_smoke)
{
    draw_smoke = new_draw_smoke;
    if (!draw_smoke) {
        draw_vecs = true;
    }
    else{draw_slices = false;}

}

void MyGLWidget::drawHedgehogs(bool new_draw_vecs)
{
    draw_vecs = new_draw_vecs;
    if (!draw_vecs) {draw_smoke = true;}
    else{draw_slices = false;}
}

void MyGLWidget::drawGrid(bool new_draw_grid)
{
    draw_grid = new_draw_grid;
}

void MyGLWidget::scaleColor(bool new_scale_color)
{
    scale_color = new_scale_color;
}

void MyGLWidget::timestep(int position)
{
    // dt start = 0.4
    //      case 't': simulation.set_dt(simulation.get_dt() - 0.001); break;
    //      case 'T': simulation.set_dt(simulation.get_dt() + 0.001); break;
    static int last_pos_timestep = 500;				//remembers last slider location, statics only get initialized once, after that they keep the new value
    double new_pos = position - last_pos_timestep;
    double old_dt = simulation.get_dt();
    double new_dt = old_dt + new_pos * 0.001; //easier to debug on separate line
    if (new_dt < 0){
        new_dt = 0;
    }
    simulation.set_dt(new_dt);
    last_pos_timestep = position;
}

void MyGLWidget::setGridSize(int position)
{
    grid_scale = position;
}

void MyGLWidget::hedgehogScaling(int position)
{
    // vec_scale = 1000;
    //  	  case 'S': vec_scale *= 1.2; break;
    //        case 's': vec_scale *= 0.8; break;
    // The scaling goes exponential with keyboard, but with slide can just do linear
    if (glyphs == "hedgehogs"){
        static int last_pos_hedgehog = 500;				//remembers last slider location
        int new_pos = position - last_pos_hedgehog;
        hedgehog_scale = hedgehog_scale + new_pos * 200; //easier to debug on separate line
        if (hedgehog_scale < 0){
            hedgehog_scale = 0;
        }
        if (hedgehog_scale > 4000){
            hedgehog_scale = 4000;
        }
        last_pos_hedgehog = position;
    }
    if (glyphs == "arrows"){
        static int last_pos_arrow = 500;				//remembers last slider location
        int new_pos = position - last_pos_arrow;
        arrow_scale = arrow_scale + new_pos*2; //easier to debug on separate line
        if (arrow_scale < 0){
            arrow_scale = 0;
        }
        if (arrow_scale > 2000){
            arrow_scale = 2000;
        }
        last_pos_arrow = position;
    }
}

void MyGLWidget::fluidViscosity(int position)
{
    // visc = 0.001
    //      case 'V': simulation.set_visc(simulation.get_visc()*5); break;
    //      case 'v': simulation.set_visc(simulation.get_visc()*0.2); break;
    // The scaling goes exponential with keyboard, but with slide can just do linear
    static int last_pos_visc = 500;
    int new_pos = position - last_pos_visc;
    double old_visc = simulation.get_visc();
    double new_visc = old_visc + new_pos * 0.005; //easier to debug on separate line
    if (new_visc < 0){
        new_visc = 0;
    }
    simulation.set_visc(new_visc);
    last_pos_visc = position;
}

void MyGLWidget::setNumberOfGlyphs(int position)
{
    number_of_glyphs = position;
}

void MyGLWidget::clampColorMin(int min_color)
{
    if (min_color > 0){
        if(dataset == "fluid density"){
            color_clamp_min_matter = min_color/100.0;
        }
        if(dataset == "fluid velocity magnitude" || dataset == "force field magnitude"){
            color_clamp_min_glyph = min_color/100.0;
        }
    }
}

void MyGLWidget::clampColorMax(int max_color)
{
    if (max_color > 0){
        if(dataset == "fluid density"){
            color_clamp_max_matter = 1-(max_color/100.0);
        }
        if(dataset == "fluid velocity magnitude" || dataset == "force field magnitude"){
            color_clamp_max_glyph = 1-(max_color/100.0);
        }
    }
}

void MyGLWidget::scalarColoring(QString scalartype){
    if (scalartype == "rainbow") {
        if (dataset == "fluid density"){
            scalar_col = 1;
        }
        else if (dataset == "fluid velocity magnitude"){
            velocity_color = 1;
        }
        else if (dataset == "force field magnitude"){
            force_field_color = 1;
        }
    }
    else if (scalartype == "black&white") {
        if (dataset == "fluid density"){
            scalar_col = 0;
        }
        else if (dataset == "fluid velocity magnitude"){
            velocity_color = 0;
        }
        else if (dataset == "force field magnitude"){
            force_field_color = 0;
        }
    }
    else if (scalartype == "heatmap") {
        if (dataset == "fluid density"){
            scalar_col = 2;
        }
        else if (dataset == "fluid velocity magnitude"){
            velocity_color = 2;
        }
        else if (dataset == "force field magnitude"){
            force_field_color = 2;
        }
    }
}

void MyGLWidget::setFluidDensity(){
    dataset = "fluid density";
}

void MyGLWidget::setFluidVelocity(){
    dataset = "fluid velocity magnitude";
}

void MyGLWidget::setForceField(){
    dataset = "force field magnitude";
}

// Color map explained
// http://www.glprogramming.com/red/chapter04.html just above table 4.2
// The first float is the offset color to start the map of R,G,B from

void MyGLWidget::drawBar(){
    glPushMatrix ();
    glBegin (GL_QUADS);
    if (draw_smoke){
        for (int i = 0; i < 1001; i = i + 1){
            set_colormap(0.001*i,scalar_col, color_clamp_min_matter, color_clamp_max_matter, color_bands, hue);
            glVertex3f(15+(0.5*i), 40, 0); //(x,y top left)
            glVertex3f(15+(0.5*i), 10, 0); //(x,y bottom left)
            glVertex3f(15+(0.5*(i+1)),10, 0); //(x,y bottom right)
            glVertex3f(15+(0.5*(i+1)),40, 0); //(x,y top right)
        }
    }
    if (draw_vecs){
        for (int i = 0; i < 1001; i = i + 1){
            set_colormap(0.001*i,velocity_color, color_clamp_min_glyph, color_clamp_max_glyph, color_bands, hue);
            glVertex3f(15+(0.5*i), 70, 0); //(x,y top left)
            glVertex3f(15+(0.5*i), 40, 0); //(x,y bottom left)
            glVertex3f(15+(0.5*(i+1)),40, 0); //(x,y bottom right)
            glVertex3f(15+(0.5*(i+1)),70, 0); //(x,y top right)
        }
    }
    glEnd ();
    glPopMatrix ();
    OGL_Draw_Text();
}

void MyGLWidget::OGL_Draw_Text(){
    //glPushMatrix();
    //glDisable(GL_LIGHTING);
    glDisable(GL_DEPTH_TEST);
    if (draw_smoke){
        //qglColor(Qt::white);
        set_colormap(1-color_clamp_min_matter,scalar_col, color_clamp_min_matter, color_clamp_max_matter,color_bands, hue);
        renderText(20, 15, 0, QString::number(color_clamp_min_matter), QFont("Arial", 12, QFont::Bold, false) ); // render bottom bar left
        //qglColor(Qt::black);
        renderText(240, 15, 0, "matter", QFont("Arial", 8, QFont::Bold, false) );
        set_colormap(1-color_clamp_max_matter, scalar_col, color_clamp_min_matter, color_clamp_max_matter, color_bands, hue);
        renderText(470, 15, 0, QString::number(color_clamp_max_matter), QFont("Arial", 12, QFont::Bold, false) ); // render bottom bar right
    }
    //QString maxCol = QString::number(color_clamp_max);
    if (draw_vecs){
        set_colormap(1-color_clamp_min_glyph,velocity_color, color_clamp_min_glyph, color_clamp_max_glyph,color_bands, hue);
        renderText(20, 45, 0, QString::number(color_clamp_min_glyph), QFont("Arial", 12, QFont::Bold, false) ); // render top bar left
        renderText(240, 45, 0, "glyph", QFont("Arial", 8, QFont::Bold, false) );
        set_colormap(1-color_clamp_max_glyph,velocity_color, color_clamp_min_glyph, color_clamp_max_glyph,color_bands, hue);
        renderText(470, 45, 0, QString::number(color_clamp_max_glyph), QFont("Arial", 12, QFont::Bold, false) ); // render top bar right
    }
    glEnable(GL_DEPTH_TEST);
    //glEnable(GL_LIGHTING);
    //glPopMatrix();

}

void MyGLWidget::setHue(int new_hue){
    hue = new_hue;
}

void MyGLWidget::setColorBands(int new_color_bands){
    color_bands = new_color_bands;
}

void MyGLWidget::setDim(int new_DIM){
    DIM = new_DIM;
    simulation.init_simulation(DIM);
}

void MyGLWidget::setGlyphType(QString new_glyphs){
    glyphs = new_glyphs;
}

void MyGLWidget::drawGridLines(int DIM){
    glBegin(GL_LINES);
    for(int i=0;i <= DIM/grid_scale;i++) {
        glColor3f(1,1,1);
        glVertex2f(i*cell_width*grid_scale,0);
        glVertex2f(i*cell_width*grid_scale,DIM*cell_height*grid_scale);
        glVertex2f(0,i*cell_height*grid_scale);
        glVertex2f(DIM*cell_width*grid_scale,i*cell_height*grid_scale);
    };
    glEnd();
}

void MyGLWidget::setDrawGradient(bool new_gradient){
    gradient = new_gradient;
    if (gradient){
        draw_slices = false;
    }
}

void MyGLWidget::setDrawStreamline(bool new_streamline){
    draw_streamline = new_streamline;
    if (draw_streamline) {
        draw_vecs = false;
        draw_slices = false;
        draw_smoke = false;
    }
}

void MyGLWidget::setDrawSlices(bool new_slices){
    draw_slices = new_slices;
    if (draw_slices) {
        draw_vecs = false;
        draw_streamline = false;
        draw_smoke = false;
    }
    if (!draw_slices) {
        draw_vecs = true;
        draw_smoke = true;
    }
}
