#ifndef COMPONENT_H_
#define COMPONENT_H_
#include "../ModelingTime.cpp"

class Component{
	protected:
		double matrix[2][3];
	public:
		virtual double* include(double value) = 0;
};

#endif