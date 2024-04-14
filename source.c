#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <omp.h>
#include <time.h>
#include <string.h>
typedef struct {
	double x;
	double y;
	double z;
	double result;
} point_t, *Point;

/* slide the volumn to select points  */
#define X_SLIDES 12
#define Y_SLIDES 10
#define Z_SLIDES 20
#define NUM_POINTS X_SLIDES * Y_SLIDES * Z_SLIDES
#define NUM_SHOTS 10000.0 /* Must be a float number */
#define FAILURE_RADIUS 1e-7 /* When radius less than this, it fails, we try again */
#define FAILURE_BOUND 500 /* Used for kill point that failure on this number */
#define PI 3.14159
#define PRECISE 1e-6

#define write_result(point, mark) \
	do { \
		if (mark == -1) point->result = -1; \
		else point->result = mark/NUM_SHOTS; \
	} while(0)

#define square_length(x, y) (x)*(x)+(y)*(y)
#define min3(result, l1, l2, l3) \
	do { \
		result = l1>=l2? (l3>=l2?l2:l3) : (l3>=l1?l1:l3); \
	} while(0)
Point create_point(double, double, double);
Point copy_point(Point);
Point* get_points(int*);
void destroy_points(Point*, int);
void print_points(Point*, int, char*, int*);
bool find_min_radius(Point pt, double*);
void move_to_next(Point, double);
void bounce_inside(Point);
bool isOnPlanes(Point, int*);

#ifdef TEST
#include "test.c"
#else
int main(int argc, char** argv) {
	if (argc == 1) {
		fprintf(stderr, "Usage: ./prog parameter\n");
		return -1;
	}
	time_t rawtime;
  	struct tm * timeinfo;
  	time( &rawtime );
  	timeinfo = localtime( &rawtime );
  	printf( "Current local time and date: %s", asctime (timeinfo) );
	printf( "Every point has %lf shots\n", NUM_SHOTS);
	//Get a list of points
	int num_of_pts;
	Point* list = get_points(&num_of_pts);
	srand(time(NULL));
	omp_set_num_threads(200);
	int total_section = atoi(argv[1]);
	int section = atoi(argv[2]);
	int part = num_of_pts/(total_section-1);
	int start = section * part;
	int end = start + part;
	end = end>num_of_pts?num_of_pts:end;
	printf("This job is %d to %d\n", start+1, end);
	#pragma omp parallel for schedule(dynamic)
	for (int i = start; i < end; i++) {
		//DO hard work
		int mark = 0;
		for (int j = 0; j < NUM_SHOTS; j++) {
			int failure = 0;
			Point copy = copy_point(list[i]);
			//fprintf(stderr, "Start %d:%d shots... ", i, j);
			while(1) {
				if (failure >= FAILURE_BOUND) {
					//fprintf(stderr, "Failure.\n");
					break;
				}
				double r = 0;
				if (find_min_radius(copy, &r) == false) {
					failure++;
					copy->x = list[i]->x;
					copy->y = list[i]->y;
					copy->z = list[i]->z;
					continue;
				}
				move_to_next(copy, r);
				bounce_inside(copy);
				if (isOnPlanes(copy, &mark)) break;
			}
			free(copy);
			if (failure >= FAILURE_BOUND) { //Kill the point
				mark = -1;
				break;
			}
		}
		write_result(list[i], mark);
	}
	int fail_count = 0;
	print_points(list+start, end-start, argv[2], &fail_count);
	destroy_points(list, num_of_pts);
	time( &rawtime );
        timeinfo = localtime( &rawtime );
	printf("Totally %d points, fails %d points.\n", end - start, fail_count);
        printf( "Finish time and date: %s\n", asctime (timeinfo) );
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

void print_points(Point* list, int num_of_pts, char* str, int* fail_count) {
	char name[15] = "points";
	strcat(name, str);
	strcat(name, ".txt");
	FILE* fd = fopen(name, "w");
	int fail_counter = 0;
	for (int i = 0; i < num_of_pts; i++) {
		Point pt = list[i];
		if (list[i]->result == -1) {
			fail_counter++;
			continue;
		}
		fprintf(fd, "%1.6lf, %1.6lf, %1.6lf, %1.6lf\n", pt->x, pt->y, pt->z, pt->result);
	}
	fclose(fd);
	*fail_count = fail_counter;
}


bool find_min_radius(Point pt, double* r) {
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
	double min = 0;
	min3(min, l1, l2, l3); //minimal square of distance to lines
	if (min <= FAILURE_RADIUS) return false;
	min3(min, 2-z, 3-x, sqrt(min)); //minimal distance to lines and also two planes
	*r = min;
	//printf("Find radius %1.6lf\n", *r);
	return true;
}

void move_to_next(Point pt, double r) {
	double u1 = (double)rand()/(RAND_MAX);
	double u2 = (double)rand()/(RAND_MAX);
	double u3 = (double)rand()/(RAND_MAX);
	double u4 = (double)rand()/(RAND_MAX);
	double z1 = sqrt(-2*log(u1))*cos(2*PI*u2);
	double z2 = sqrt(-2*log(u1))*sin(2*PI*u2);
	double z3 = sqrt(-2*log(u3))*cos(2*PI*u4);
	double norm = sqrt(z1*z1 + z2*z2 + z3*z3);
	
	pt->x += r * (z1/norm);
	pt->y += r * (z2/norm);
	pt->z += r * (z3/norm);
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

	//printf("Move to %.6lf, %.6lf, %.6lf\n", pt->x, pt->y, pt->z);
}

bool isOnPlanes(Point pt, int* mark) {
	if (pt->x <= 3 + PRECISE && pt->x >= 3 - PRECISE) {
		(*mark)++;
		#ifndef TEST
		//fprintf(stderr, "Right plane\n");
		#endif
		return true;
	}
	else if (pt->z <= 2 + PRECISE && pt->z >= 2 - PRECISE) {
		#ifndef TEST
		//fprintf(stderr, "Up plane\n");
		#endif
		return true;
	}
	return false;
}

