#include <vector>
#include <iostream>
#include "Components/Component.h"

using namespace std;

class Node;

class AddableComponent : public Component{
	private:
		Component* component;
		Node* inNode;
		Node* outNode;
	public:
		AddableComponent(Component* component , Node* first , Node* second){
			this -> component = component;
			this -> inNode = first;
			this -> outNode = second;
		}

		~AddableComponent(){
			delete component;
		}

		double* include(double value){
			return component -> include(value);
		}

		Node* getFirstNode(){
			return inNode;
		}

		Node* getSecondNode(){
			return outNode;
		}
};

class Node{
	private:
		int numNode;
		double* states;
		vector<AddableComponent*> components;
	public:
		Node(int &numNode , double* states){
			this -> numNode = numNode;
			this -> states = states;
		}

		int getNumNode(){
			return numNode;
		}

		void addComponent(AddableComponent* component){
			components.push_back(component);
		}

		void createStiffMatrix(double **mtr , int length , int y){
			double* array = mtr[0];
			for(vector<AddableComponent*>::iterator iterator = components.begin() ; iterator != components.end() ; iterator++){
				AddableComponent* component = *iterator;
				int out = component -> getFirstNode() -> getNumNode();
				int in = component -> getSecondNode() -> getNumNode();
				double* matrix = component -> include(states[in] - states[out]);
				if(out == numNode){
					array[numNode] += matrix[0];
					array[in] += matrix[1];
					array[length - 1] += matrix[2];
				}
				else{
					array[out] += matrix[3];
					array[numNode] += matrix[4];
					array[length - 1] += matrix[5];
				}
			}
		}
};

