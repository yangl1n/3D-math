#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <time.h>

typedef struct {
	double x;
	double y;
	double z;
	double result;
} point_t, *Point;

#ifndef BATCH
#define X_SLIDES 2
#define Y_SLIDES 2
#define Z_SLIDES 2
#endif

#define NUM_POINTS X_SLIDES * Y_SLIDES * Z_SLIDES
#define NUM_SHOTS 100.0 /* Must be a float number */
#define write_result(point, mark) \
	do { \
		point->result = mark/NUM_SHOTS; \
	} while(0)
#define square_length(x, y) (x)*(x)+(y)*(y)
#define sqrt_min(l1, l2, l3) \
	do { \
		r = l1>=l2? (l3>=l2?l2:l3) : (l3>=l1?l1:l3); \
		r = sqrt(r); \
	} while(0)
#define PI 3.14159
#define PRECISE 1e-6
Point create_point(double, double, double);
Point copy_point(Point);
Point* get_points(int*);
void destroy_points(Point*, int);
void print_points(Point*, int);
double find_min_radius(Point pt);
void move_to_next(Point, double);
void bounce_inside(Point);
bool isOnPlanes(Point, int*);

#ifdef TEST
#include "test.c"
#else
int main() {
	//Get a list of points
	int num_of_pts;
	Point* list = get_points(&num_of_pts);
	srand(time(NULL));
	for (int i = 0; i < num_of_pts; i++) {
		//DO hard work
		int mark = 0;
		for (int j = 0; j < NUM_SHOTS; j++) {
			Point copy = copy_point(list[i]);
			printf("Start %d:%d shots\n", i, j);
			while(1) {
				double r = find_min_radius(copy);
				if (r <= 0+1e-6) return -1;
				move_to_next(copy, r);
				bounce_inside(copy);
				if (isOnPlanes(copy, &mark)) break;
			}
			free(copy);
		}
		write_result(list[i], mark);
	}
	print_points(list, num_of_pts);
	destroy_points(list, num_of_pts);
	return 0;
}
#endif /* Switch between test and release mode */

Point copy_point(Point pt) {
	Point new = create_point(pt->x, pt->y, pt->z);
	new->result = pt->result;
	return new;
}

Point create_point(double x, double y, double z) {
	Point new = malloc(sizeof(*new));
	new->x = x;
	new->y = y;
	new->z = z;
	new->result = 0;
	return new;
}
	
Point* get_points(int* num_of_pts) {
	int counter = 0;
	double x_d = 3.0/X_SLIDES;
	double y_d = 1.0/Y_SLIDES;
	double z_d = 2.0/Z_SLIDES;
	Point* list = malloc(sizeof(*list) * NUM_POINTS);
	for (int i = 0; i < NUM_POINTS; i++) list[i] = NULL;
	for (int i = 1; i < X_SLIDES; i++) {
		double x = i * x_d;
		for (int j = 1; j < Y_SLIDES; j++) {
			double y = j * y_d;
			for (int k = 1; k < Z_SLIDES; k++) {
				double z = k*z_d;
				//check valid
				if (x > 1 && z > 1) continue; //Not in box
				else if (x == 1 && z == 1) continue;
				list[counter++] = create_point(x, y, z);
			}
		}
	}
#ifndef TEST
	printf("Generate %d points.\n", counter);
#endif
	*num_of_pts = counter;
	return list;
}

void destroy_points(Point* list, int num_of_pts) {
	for (int i = 0; i < num_of_pts; i++)
		free(list[i]);
	free(list);
}

void print_points(Point* list, int num_of_pts) {
	for (int i = 0; i < num_of_pts; i++) {
		Point pt = list[i];
		printf("%d: %1.6lf, %1.6lf, %1.6lf, %1.6lf\n", i+1, pt->x, pt->y, pt->z, pt->result);
	}
}


double find_min_radius(Point pt) {
	double l1, l2, l3;
	double z = pt->z;
	double x = pt->x;
	double y = pt->y;
	l1 = square_length(z-1, x-1);
	if (pt->z >= 1 && pt->z <= 2 && pt->x <= 1) {
		if (pt->y <= 1 && pt->y >= 0.5) {
			l2 = square_length(x, 1-y);
			l3 = square_length(1-x, 1-y);
		}
		else {
			l2 = square_length(x, y);
			l3 = square_length(1-x, y);
		}
	}
	else if (pt->x >= 0 && pt->x <= 1) {
		if (x+z-1 < 0) l1 = square_length(x, z);
		if (pt->y >= 0.5 && pt->y <= 1) {
			l2 = square_length(x, 1-y);
                        l3 = square_length(z, 1-y);
		}
		else {
			l2 = square_length(x, y);
                        l3 = square_length(y, z);
		}
	}
	else {
		if (pt->y >= 0.5 && pt->y <= 1) {
			l2 = square_length(1-y, 1-z);
                        l3 = square_length(1-y, z);
		}
		else {
			l2 = square_length(1-z, y);
                        l3 = square_length(z, y);
		}
	}
	double r = 0;
	sqrt_min(l1, l2, l3);
	printf("Find radius %1.6lf\n", r);
	return r;
}

void move_to_next(Point pt, double r) {
	double theta = PI*rand()/(RAND_MAX);
	double phi = 2*PI*rand()/(RAND_MAX);
//	printf("Theta %1.6lf, phi %1.6lf\n", theta, phi);
	pt->x += r*sin(theta)*cos(phi);
	pt->y += r*sin(theta)*sin(phi);
	pt->z += r*cos(theta);
}

void bounce_inside(Point pt) {
	//Check x
	if (pt->x < 0) pt->x = -(pt->x);
	else if (pt->x > 1 && pt->z > 1) pt->z = 2.0 - (pt->z);
	else if (pt->x > 3) pt->x = 6 - (pt->x);
	//Check y
	if (pt->y < 0) pt->y = -(pt->y);
	else if (pt->y > 1) pt->y = 2 - (pt->y);
	//Check z
	if (pt->z < 0) pt->z = -(pt->z);
	else if (pt->z > 2) pt->z = 4 - (pt->z);

	printf("Move to %.6lf, %.6lf, %.6lf\n", pt->x, pt->y, pt->z);
}

bool isOnPlanes(Point pt, int* mark) {
	if (pt->x <= 3 + PRECISE && pt->x >= 3 - PRECISE) {
		(*mark)++;
		#ifndef TEST
		printf("Right plane\n");
		#endif
		return true;
	}
	else if (pt->z <= 2 + PRECISE && pt->z >= 2 - PRECISE) {
		#ifndef TEST
		printf("Up plane\n");
		#endif
		return true;
	}
	return false;
}

