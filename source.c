#include <stdio.h>
typedef struct {
	double x;
	double y;
	double z;
	double result;
} point_t, *Point;

#define NUM_POINTS 100
#define NUM_SHOTS 10000
#define write_result(point, mark) \
	do { \
		point->result = mark/NUM_SHOTS; \
	} while(0)

Point create_point(Point);
Point copy_point(Point);

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
	
	
int main() {
	//Get a list of points
	Point list = get_points();
	for (int i = 0; i < NUM_POINTS; i++) {
		//DO hard work
		int mark = 0;
		Point copy = copy_point(list[i]);
		for (int j = 0; j < NUM_SHOTS; j++) {
			while(1) {
				double r = find_min_radius(copy);
				copy = next_position(list[i], r);
				if (isOnPlanes(copy, &mark)) break;
			}
		}
		write_result(list[i], mark);
	}
	print_points(list);
	destory_points(list);
	return 0;
}
