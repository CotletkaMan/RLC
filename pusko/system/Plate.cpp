#include "Node.cpp"
#include <vector>
#include <iostream>

class Plate{
	private:
		int X;
		int Y;
		int countNodes;
		double* voltage;
		Node** nodes;
		int* inOrder;
		std::vector<Component*> components;

	public:
		Plate(int x , int y , double* voltage){
			this -> X = x;
			this -> Y = y;
			this -> voltage = voltage;
			countNodes = (X * Y) + 1;
			nodes = new Node*[countNodes];
			inOrder = new int[countNodes];
			for(int i = 0 ; i < countNodes ; i++){
				inOrder[i] = i;
				nodes[i] = new Node(inOrder[i] , &voltage[0]);
			}
		}

		~Plate(){
			for(int i = 0 ; i < countNodes ; i++)
				delete nodes[i];
			delete nodes;
			delete inOrder;
			for(int i = 0 ; i < components.size() ; i++)
				delete components[i];
		}

		bool validateNode(int Node){
			return Node >= 0 && Node < countNodes;
		}

		void addComponent(int in , int out , Component* component){
			if(validateNode(in) && validateNode(out)){
				AddableComponent* addComponent = new AddableComponent(component , nodes[in] , nodes[out]);
				nodes[in] -> addComponent(addComponent);
				nodes[out] -> addComponent(addComponent);
				components.push_back(addComponent);
			}
		}

		void createStiffMatrix(double **mtr , int length , int y){
			for(int i = 0 ; i < y ; i++){
				nodes[i] -> createStiffMatrix(&mtr[i] , length , y);
			}
		}

		Node** getNodes(){
			return nodes;
		}
};
