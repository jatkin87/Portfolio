#ifndef OPTIMIZER_H
#define OPTIMIZER_H
#include <cmath>
#include <map>
#include <utility>
#include <vector>
//c++ won't let me const static maps, so it is a global. For Nodes use
// I didn't want to reinitialize this map for every single node
map<string,int> ord
{
    {"+",0},
    {"-",0},
    {"*",1},
    {"/",1},
    {"^",2},
    {"<<",5}, //This will be how I store function calls
    {"=",-2}, //assignment operator
    //ternaries and booleans
    {"?",-1}, //less priority than =, but more priority than anything else
    {"==",3}, //set these to be less priority than operators
    {"<=",3},
    {">=",3},
    {"!=",3},
    {"<",3},
    {">",3},
    {";",4}
};

//Expression optimizer
class Optimizer
{
private:
    //Node class for internal tree structure
    class Node
    {
    private:
        Node * left {nullptr};
        Node * right {nullptr};

    public:
        //initialize to *, parenthesis need to be 1*(expr) or (expr)*1 for algorithm to work
        pair<string,string> value {"operator","*"};
        string nasmExpr = ""; 
        /**
         * clean up like a good boy
         */
        ~Node()
        {
            if(left)
            {
                delete left;
                left = nullptr;
            }
            if(right)
            {
                delete right;
                right = nullptr;
            }

        }
        //ain't going to be copying nodes, so lets get rid of the copy constructors
        Node (const Node&) = delete;
        Node& operator= (const Node&) = delete;
        Node() = default;
        /**
         * The idea behind this is that I need to decide which values goes to the left
         * and which values go to the right, before actualy creating the left and right
         * nodes. The algorithm below finds the operator that needs to be on top, and everything
         * to the left of that operator goes to the left, and everything to the right goes to
         * the right. I then pass those tokens into the left and right nodes and do the same thing.
         * I have a few rules for parenthesis and nodes with only one child. I also make sure every
         * branch node has two children.
         * 
         * @param tokens a vector of pairs of strings from tokenizer.h class
         */
        Node(vector<pair<string,string>> tokens)
        {
            //If we only have one token left
            if(tokens.size() == 1)
            {
                //set the value and return
                value = tokens[0];
                return;
            }
            //This happens for = operator. something like "int var"
            if(tokens.size() == 2 && tokens[0].first == "type" && tokens[1].first == "name")
            {
                value = pair<string,string>{tokens[0].second, tokens[1].second};
                return;
            }

            bool topOp = false; //to determine whether we have found the top operator
            bool lastTokenOp = false; // to check if the last token was an operator
            auto leftStuff = new vector<pair<string,string>>(); //stuff we will send to the left
            auto rightStuff = new vector<pair<string,string>>(); //stuff we will send to the right
            //the "direction", or container we are currently putting values into
            vector<pair<string,string>> *direction = leftStuff;
            //the opposite of direction
            vector<pair<string,string>> *opp = rightStuff;
            //go through each token
            for(int i = 0; i < tokens.size(); i++)
            {
                if(tokens[i].first == "comment")
                {
                    continue;
                }
                //Basically, if the first value is a -, we want that to be noted
                if(i == 0 && tokens[i].first == "operator" && tokens[i].second != "<<")
                {
                    lastTokenOp = true;
                    direction->push_back({"int32", to_string(ord[tokens[i].second])});
                    direction->push_back(tokens[i]);
                    continue;
                }
                if(i == 0 && tokens[i].second == "<<")
                {
                    lastTokenOp = true;
                    topOp = true;
                    direction->push_back({"function", "print"});
                    this->value = tokens[i];
                    direction = rightStuff;
                    opp = leftStuff;
                    continue;
                }
                //if the token is an operator
                if(tokens[i].first == "operator" || tokens[i].first == "ternary")
                {
                    if(!topOp && !lastTokenOp) //if we haven't found the top operator yet
                    {
                        value = tokens[i]; //set the top operator
                        topOp = true; //set to true, we found it
                        //switch directions
                        //all the stuff prior to this operator is to the left,
                        // so all the other stuff needs to go to the right
                        direction = rightStuff; 
                        opp = leftStuff;
                        lastTokenOp = true; //so we know the last iteration was an operator
                        continue; //go to next token
                    }
                    //If we have found the top operator, the last token wasn't an operator
                    // and the order of the operator is equal to or less than the top operator
                    // I also change to to less than if we are half way through the expression
                    // so that the tree is more likely to be balanced.
                    if(topOp && !lastTokenOp && (ord[tokens[i].second] <= ord[value.second]))
                    {
                        //because the operator is changing, everything prior to it
                        // needs to go to the left of the operator
                        //take the old top operator and put it into the left side
                        opp->push_back(value);
                        //make the new top operator the current token
                        value = tokens[i];
                        //put all of right's values into left
                        Merge(*opp,*direction);
                        //last token was an operator
                        lastTokenOp = true;
                        //go to next token
                        continue;
                    }
                    //the last token was an operator
                    lastTokenOp = true;
                }
                //if the token is "("
                if(tokens[i].second == "(")
                {
                    //We need to keep everything in the parenthesis together
                    // we are going to kick it to the next node to deal with
                    lastTokenOp = false; // if the last was a token, it isn't anymore
                    //open represents how many times we have seen a "(", we will make internal parenthesis the next node's problem
                    int open = 1;
                    auto pExpr = new vector<pair<string,string>>();
                    if(topOp)
                    {
                        //basically if the parenthesis is not at the beginning, we keep the parenthesis for another node's problem
                        pExpr->push_back(tokens[i]);
                    }
                    i++; //go to next token
                    while(open > 0)
                    {
                        //every time we find an open parenthesis, we need to keep track
                        open += tokens[i].second == "(";
                        //put the token into the container
                        pExpr->push_back(tokens[i]);
                        i++; //next token
                        //if the token is a closed parenthesis, subtract from open
                        open -= tokens[i].second == ")";
                        //this will make sure we are done when we have completed the outermost group
                    }
                    if(topOp)
                    {
                        //same as above, keep the closing parenthesis if the group isn't at the start
                        pExpr->push_back(tokens[i]);
                    }
                    if(!topOp && i+1 < tokens.size() && tokens[i+1].first == "operator")
                    {
                        direction->push_back({"bracket", "("});
                        pExpr->push_back(tokens[i]);
                    }
                    Merge(*direction,*pExpr);
                    delete pExpr;
                    pExpr = nullptr;
                    //go to next token
                    continue;
                }
                //if you made it here, you are a num or a name
                lastTokenOp = false; //not an operator
                direction->push_back(tokens[i]); //put the token in
            }

            if(!topOp && tokens.size() == 2 && tokens[0].first != "operator")
            {
                throw runtime_error("Error processing expression");
            }
            //if there is nothing in the two sides, we put in it's order value
            // for example, a -4 will have left empty, so we add a 0 so it becomes 0-4
            if(opp->size() == 0)
            {
                opp->push_back({"int32",to_string(ord[value.second])});
            }
            if(direction->size() == 0 && value.second != ";")
            {
                direction->push_back({"int32",to_string(ord[value.second])});
            }
            else if(direction->size() == 0 && value.second == ";")
            {
                direction->push_back({"name", "void"});
            }
            //send all the left stuff to the new left node
            left = new Node(*leftStuff);
            //send all the right stuff to the new right mode.
            right = new Node(*rightStuff);
            
            //cleanup our junk
            direction = nullptr; //leftStuff still has the pointer
            opp = nullptr; //rightStuff still has the pointer
            delete leftStuff;
            leftStuff = nullptr;
            delete rightStuff;
            rightStuff = nullptr;
        }
        /**
         * gets the node value
         * 
         * @return pair<string,string> value
         */
        pair<string,string> getValue()
        {
            return value;
        }
        /**
         * sets the value
         * 
         * @param x pair<string,string> value
         */
        void setValue(pair<string,string> x)
        {
            value = x;
        }
        /**
         * merges two vectors, clears the second vector
         * 
         * @param a address of collection being merged into
         * @param b address of collection being merged from, will be cleared
         */
        void Merge(vector<pair<string,string>> &a, vector<pair<string,string>> &b)
        {
            a.insert(a.end(), b.begin(), b.end());
            b.clear();
        }
        /**
         * returns the expression in postfix order
         * 
         * @return string postfix order of tree
         */
        string process()
        {
            string ret = "";
            if(left)
            {
                ret += left->process();
            }
            if(right)
            {
                ret += right->process();
            }
            return ret + value.second + " ";
        }

        void process(SymbolTable * st, bool leftOfEq = false, bool rightOfFunc = false)
        {
            if(left)
                left->process(st, value.second == "=");
            if(right)
            {
                if(!(value.second == "=") && st->exists(left->nasmExpr) && !isdigit(left->nasmExpr.at(0)))
                {
                    st->st[left->nasmExpr].inc();
                    st->dependsOn.push_back(left->nasmExpr);
                    left->nasmExpr = st->st[left->nasmExpr].dependsOn[st->st[left->nasmExpr].depIndex];
                }
                right->process(st, false, value.second == "<<");
                if(!(value.second == "=") && st->exists(right->nasmExpr) && !isdigit(right->nasmExpr.at(0)))
                {
                    st->st[right->nasmExpr].inc();
                    st->dependsOn.push_back(right->nasmExpr);
                    right->nasmExpr = st->st[right->nasmExpr].dependsOn[st->st[right->nasmExpr].depIndex];
                }
            }

            if(value.first == "operator" || value.first == "ternary")
            {
                nasmExpr = "(" + left->nasmExpr + value.second + right->nasmExpr + ")";
                return;
            }
            
            nasmExpr = value.second;
            if(!rightOfFunc && !leftOfEq && (atoi(nasmExpr.c_str()) == 0 && !(nasmExpr == "0")) && st->exists(nasmExpr))
            {
                
                nasmExpr = st->getSymbol(nasmExpr)->dependsOn[st->getSymbol(nasmExpr)->depIndex];
                
            }
            if(rightOfFunc)
            {
                string temp = value.second;
                string type = "";
                while(st->exists(temp))
                {
                    type = st->st[temp].type;
                    temp = st->st[temp].dependsOn[st->st[temp].depIndex];
                }
                //if(temp == nasmExpr)
                //{
                //    throw runtime_error("Symbol " + nasmExpr + " has not been declared yet.");
                //}
                nasmExpr = temp;
                value.first = type;
            }
        }

        string processInFix()
        {
            string ret = "";
            if(left)
            {
                ret += "(" + left->processInFix() + ")";
            }
            ret += value.second;
            if(right)
            {
                ret += "(" + right->processInFix() + ")";
            }
            
            return ret;
        }
        /**
         * Goes through the tree and evaluates expressions, replacing node branches with values
         */
        void optimize()
        {
            //if we are at an operator
            if(value.first == "operator" || value.first == "ternary")
            {
                if(value.second == ";" && right->getValue().second == "void")
                {
                    //we need to replace this node with whatever is on the left
                    Node * temp = left;
                    left = temp->getLeft();
                    temp->left = nullptr;
                    delete right;
                    right = temp->getRight();
                    temp->right = nullptr;
                    value = temp->getValue();
                    delete temp;
                }
                if(value.second == "*" && left->getValue().first == "num" && right->getValue().first == "name")
                {
                    Node * swap = left;
                    left = right;
                    right = swap;
                }
                //if left exists, but left isn't a number
                if(left && !(left->getValue().first == "int32" || left->getValue().first == "float32"))
                {
                    left->optimize(); //optimize left
                }
                //if right exists, but right isn't a number
                if(right && !(right->getValue().first == "int32" || right->getValue().first == "float32"))
                {
                    right->optimize(); //optimize right
                }
                /**
                 * The following if statements try to get rid of unknowns that we know
                 * For example, var1-var1 = 0. So lets get rid of the vars and replace them
                 * with 0.
                 */
                //If something is multiplied by 1, we can replace it with the child
                //This is for "something*1"
                if(value.second == "*" && (left && right) && right->getValue().second == "1")
                {
                    Node * temp = left;
                    right = left->right;
                    left = left->left;

                    value = temp->getValue();
                    temp->left = nullptr;
                    temp->right = nullptr;
                    delete temp;
                    temp = nullptr;
                }
                //this is for 1*something
                if(value.second == "*" && (left && right) && left->getValue().second == "1")
                {
                    Node * temp = right;
                    left = right->right;
                    right = right->left;

                    value = temp->getValue();
                    temp->left = nullptr;
                    temp->right = nullptr;
                    delete temp;
                    temp = nullptr;
                }
                //I am going to make something like var*var into var^2, so we can get rid of
                //one of the variable calls
                if(value.second == "*" && (left && right)
                && !(right->getValue().first == "operator")
                && right->getValue() == left->getValue())
                {
                    value = {"operator", "^"};
                    right->setValue({"int32", "2"});
                }
                //this cleans up stuff like a * a^2, which should be a^3
                if(value.second == "*" && (left && right)
                && !(left->getValue().first == "operator")
                && (right->getValue().second == "^")
                && (left->getValue().second == right->left->getValue().second))
                {
                    Node * temp = right; //right will be replaced with a + operator for the powers
                    value = temp->getValue(); //set value to ^
                    Node * newRight = new Node(); //Create a new right node
                    newRight->setValue({"operator", "+"}); //it will be a + operator
                    newRight->left = temp->right; //the power of the right side
                    newRight->right = new Node(); //we need to add 1
                    //We don't know whether right's power is a number or a subtree, so we need to add 1 this way
                    newRight->right->setValue({"int32", "1"});
                    //set the new right
                    right = newRight;
                    
                    //cleanup the old right
                    //left is just the base, which will be a name or num (probably a name) so we can delete it
                    delete temp->left;
                    temp->left = nullptr;
                    //right is now the left of this->right, so we let that manage the pointer
                    temp->right = nullptr;
                    //delete temp
                    delete temp;

                    //because we added a new subtree, we need to optimize
                    // if right's power is 2, we need it to become 3.
                    right->optimize();

                }
                // This is for scenarios like a^2 * a should equal a^3
                if(value.second == "*" && (left && right)
                && !(right->getValue().first == "operator")
                && (left->getValue().second == "^")
                && (right->getValue().second == left->left->getValue().second))
                {
                    //I am going to swap left and right, then just apply the above method
                    Node * swap = left;
                    left = right;
                    right = swap;

                    //apply the above method
                    Node * temp = right;
                    value = temp->getValue();
                    Node * newRight = new Node();
                    newRight->setValue({"operator", "+"});
                    newRight->left = temp->right;
                    newRight->right = new Node();
                    newRight->right->setValue({"int32", "1"});
                    right = newRight;

                    delete temp->left;
                    temp->left = nullptr;
                    temp->right = nullptr;
                    delete temp;

                    right->optimize();

                }
                //This is for something/1, which is something
                if(value.second == "/" && (left && right) && right->getValue().second == "1")
                {
                    Node * temp = left;
                    right = left->right;
                    left = left->left;

                    value = temp->getValue();
                    temp->left = nullptr;
                    temp->right = nullptr;
                    delete temp;
                    temp = nullptr;
                }
                //I thought about doing a similar optimization for something like var1/var1 = 1
                // but if var1 = 0, this isn't accurate, so I don't want that optimized

                //Anything to the power of 0 is 1, so we can replace this with 1
                if(value.second == "^" && (left && right) && right->getValue().second == "0")
                {
                    value = {"int32", "1"};
                    delete left;
                    left = nullptr;
                    delete right;
                    right = nullptr;
                }
                //power to a power should be the powers multiplied
                if(value.second == "^" && (left && right) && left->getValue().second == "^")
                {
                    Node * rightSide = new Node();
                    rightSide->setValue({"operator","*"});
                    rightSide->right = right;
                    rightSide->left = left->right;
                    right = rightSide;
                    left->right = nullptr;

                    Node * leftSide = left->left;
                    left->left = nullptr;
                    delete left;
                    left = leftSide;
                }
                // the idea is that something like var^2 * var^2 should be var^4
                // However, if it is var^(1-var2)*var^(2+var2) should equate to
                // var^((1-var2)+(2+var2))
                if(value.second == "*" && (left && right) 
                && (left->getValue().second == "^" && right->getValue().second == "^") 
                && (left->left->getValue().first != "operator")
                && (left->left->getValue().second == right->left->getValue().second))
                {
                    value.second = "^";
                    
                    //power should be left's right and right's right added together
                    Node * power = new Node();
                    power->setValue({"operator", "+"});
                    //hand off children
                    power->left = left->right;
                    left->right = nullptr;
                    power->right = right->right;
                    right->right = nullptr;
                    //I probably don't need to null it first, but I like being safe
                    right = nullptr;
                    right = power;
                    //we don't need left's right anymore because it is now in the power node
                    delete left->right;
                    left->right = nullptr;

                    //we need to replace left with the base, which is left's left
                    Node * base = left->left;
                    left->left = nullptr;
                    delete left;
                    left = base;
                    
                    //optimize the new right tree
                    right->optimize();
                }
                //var - var = 0. Want to make sure we are subtracting actual values and not
                //operators. Because a tree that is +-+ should not equate to 0
                if(value.second == "-" && (left && right)
                && !(right->getValue().first == "operator")
                && right->getValue() == left->getValue())
                {
                    value = {"int32", "0"};
                    delete left;
                    left = nullptr;
                    delete right;
                    right = nullptr;
                }
                // a+a = a*2
                if(value.second == "+" && (left && right)
                && !(right->getValue().first == "operator")
                && right->getValue() == left->getValue())
                {
                    value = {"operator", "*"};
                    right->setValue({"int32", "2"});
                }
                //lets get rid of anything that is something - 0
                if(value.second == "-" && (left && right) && right->getValue().second == "0")
                {
                    delete right;
                    right = nullptr;
                    Node * temp = left;
                    value = temp->getValue();
                    left = temp->left;
                    right = temp->right;

                    temp->left = nullptr;
                    temp->right = nullptr;
                    delete temp;
                    temp = nullptr;
                }

                /**
                 * the rest solves if it can
                 */     
                // if left or right is a float
                if(value.first != "ternary" && (left && left->getValue().first == "float32") || (right && right->getValue().first == "float32"))
                {
                    //solve as floats
                    auto lhand = stof(left->getValue().second);
                    auto rhand = stof(right->getValue().second);
                    value = {"float", solve(lhand, rhand, value.second)};
                    //we solved this branch, so delete everything under it.
                    delete left;
                    left = nullptr;
                    delete right;
                    right = nullptr;
                    return;
                }
                //if both left and right are ints
                if(value.first != "ternary" &&
                  (left && left->getValue().first == "int32" && 
                        (left->getValue().second == "0" || atoi(left->getValue().second.c_str()) != 0)) 
                && (right && right->getValue().first == "int32" && 
                        (right->getValue().second == "0" || atoi(right->getValue().second.c_str()) != 0)))
                {
                    //solve as ints
                    auto lhand = stoi(left->getValue().second);
                    auto rhand = stoi(right->getValue().second);
                    value = {"int32", solve(lhand, rhand, value.second)};
                    //get rid of left and right nodes, and their children
                    delete left;
                    left = nullptr;
                    delete right;
                    right = nullptr;
                    return;
                }
                //Just in case the numerator is a name, we still want to be invalid if denominator is 0
                if(value.second == "/" && right->getValue().second == "0")
                {
                    throw runtime_error("Can't divide by 0.");
                }
            }

        }
        void optimizePar()
        {
            if(value.second == "*" && left->getValue().first == "num" && right->getValue().first == "name")
            {
                Node * swap = left;
                left = right;
                right = swap;
            }
            if(value.second == ";" && right->getValue().second == "void")
            {
                //we need to replace this node with whatever is on the left
                Node * temp = left;
                left = temp->getLeft();
                temp->left = nullptr;
                delete right;
                right = temp->getRight();
                temp->right = nullptr;
                value = temp->getValue();
                delete temp;
            }
            //If something is multiplied by 1, we can replace it with the child
            //This is for "something*1"
            if(value.second == "*" && (left && right) && right->getValue().second == "1")
            {
                Node * temp = left;
                right = left->right;
                left = left->left;

                value = temp->getValue();
                temp->left = nullptr;
                temp->right = nullptr;
                delete temp;
                temp = nullptr;
            }
            //this is for 1*something
            if(value.second == "*" && (left && right) && left->getValue().second == "1")
            {
                Node * temp = right;
                left = right->right;
                right = right->left;

                value = temp->getValue();
                temp->left = nullptr;
                temp->right = nullptr;
                delete temp;
                temp = nullptr;
            }
            //if left exists, but left isn't a number
            if(left)
            {
                left->optimizePar(); //optimize left
                nasmExpr += left->nasmExpr;
            }
            nasmExpr += value.second;
            //if right exists, but right isn't a number
            if(right)
            {
                right->optimizePar(); //optimize right
                nasmExpr += right->nasmExpr;
            }
        }
        /**
         * solves a simple equation based on operator
         * 
         * @tparam T must be a numerical type
         * @param lhand left number
         * @param rhand right number
         * 
         * @return string representation of the solved number, since nodes can only hold strings
         */
        template<typename T, typename = typename enable_if<is_arithmetic<T>::value, T>::type> //require arthimetic T
        string solve(T lhand, T rhand, string op)
        {
            //switch the operator symbol
            //this is pretty self explanatory
            switch(op[0])
            {
                case '+':
                    return to_string(lhand + rhand);
                case '-':
                    return to_string(lhand - rhand);
                case '*':
                    return to_string(lhand * rhand);
                case '/':
                    if(rhand == 0)
                    {
                        throw runtime_error("Cannot divide by 0.");
                    }
                    return to_string(lhand / rhand);
                case '^':
                    //cast to T type to maintain floats and ints
                    return to_string((T)pow(lhand,rhand));
                //something really went wrong here
                default:
                    throw runtime_error("Error processing symbol: " + op);
            }
        }
        Node * getLeft()
        {
            return left;
        }
        Node * getRight()
        {
            return right;
        }
    };
    map<string, int> opSwitcher {
        {"+", 0},
        {"-", 1}, 
        {"*", 2}, 
        {"/", 3}, 
        {"<<", 4}, 
        {"<", 5},
        {"<=", 6},
        {">", 7},
        {">=", 8}, 
        {"==", 9},
        {";", 10},
        {"?", 11},
        {"^", 12},
        {"default", -1}};
public:
    Node * root{nullptr}; // root node of the tree
    ~Optimizer() 
    {
        delete root;
        root = nullptr;
    
    } //cleanup
    Optimizer() {};
    /**
     * class that optimizes an expression as much as it can
     * 
     * @param expression expression to be optimized
     */
    Optimizer(string expression, LL1 * ll)
    {
        //Tokenize the expression
        Tokenizer t(expression);
        if(t.getTokens().size() == 0)
        {
            root = nullptr;
            return;
        }
        if(t.getTokens()[0].first == "if")
        {
            string ifExpr = Tokenizer::formatIf(expression);
            t = Tokenizer(ifExpr);
        }   
        //Because my algorithm doesn't require the LL1 to work,
        // we check this separately. Will throw errors if the expression
        // doesn't fit into the language
        ll->parseLine(t.getTokens());
        //ll.printTable();
        //start the node building process
        if(!t.getTokens().size() == 0)
            root = new Node(t.getTokens());
    }
    Optimizer(string expression)
    {
        //Tokenize the expression
        Tokenizer t(expression);
        //start the node building process
        root = new Node(t.getTokens());
    }
    /**
     * gets the postfix order of the tree
     * 
     * @return string of the postfix order of the expression
     */
    string process()
    {
       return root->process();
    }
    /**
     * processes the tree with the symbol table, to convert known values with their address locations
     * so "int a  = 0;" would store 0 into [rbp-4], and "int b = a + 1;" will change "a" to "[rbp-4]"
     */
    void process(SymbolTable * st)
    {
        root->process(st);
    }
    /**
     * go through the tree and generate the nasm file stuff
     * 
     * @param nasm address of the NASMConverter object
     * @param loc location this particular expression belongs at
     * @param curr current node, defaults to root
     * @param move should we do a move command in nasm for the next symbol, if appropriate?
     * @param format if we are printing, we need a format to print to
     */
    void processWithNasm(NASMConverter &nasm, string loc, Node* curr = nullptr, bool move = true, string format = "", string lineNum = "")
    {
        // if I haven't given you a node, you should be root
        if(!curr)
            curr = root;
        string left = ""; //initialize the left string
        string right = ""; //initialize the right string
        if(curr->getLeft()) //if there is a left
        {
            //if we are at an operator, pass location on, if we have a value then use that as location
            left = curr->getLeft()->getValue().first == "operator" ? loc : curr->getLeft()->getValue().second;
            processWithNasm(nasm, left, curr->getLeft(), false);
        }
        if(curr->getRight()) //same logic as left
        {
            right = curr->getRight()->getValue().first == "operator" ? loc : curr->getRight()->getValue().second;
            processWithNasm(nasm, right, curr->getRight(), false);
        }
        string line = ""; //nasm writeString needs an address, so we will give it one
        string opKey = opSwitcher.find(curr->value.second) == opSwitcher.end() ? "default" : curr->value.second;
        string scopeName = "if?_" + loc;
        switch(opSwitcher[opKey])
        {
            case 0: //add
                line = nasm.Add(loc, left, right);
                nasm.writeString(line);
                break;
            case 1: //subtract
                line = nasm.Sub(loc, left, right);
                nasm.writeString(line);
                break;
            case 2: //mul
                line = nasm.Mul(loc, left, right);
                nasm.writeString(line);
                break; 
            case 3: //div
                line = nasm.Divide(loc, left, right);
                nasm.writeString(line);
                break;
            case 4: //for now print, will be function logic
                line = nasm.Print(right, format);
                nasm.writeString(line);
                break;
            case 5: //<
                line = nasm.lessThan(curr->getLeft()->nasmExpr, curr->getRight()->nasmExpr, scopeName);
                nasm.writeString(line);
                break;
            case 6: //<=
            case 7: //>
            case 8: //>=
                break;
            case 9: //==
                line = nasm.isEqual(curr->getLeft()->nasmExpr, curr->getRight()->nasmExpr, scopeName);
                nasm.writeString(line);
                break;
            case 10: //;
            case 11: //?
                break;
            case 12: //^
                line = nasm.Pow(left, right, loc, lineNum);
                nasm.writeString(line);
                break;
            default:
                line = nasm.Mov(curr->value.second, loc); //generate command
                void * trash = move ? nasm.writeString(line) : nullptr; //only write it if move is required
                break;
        }
        
    }
    /**
     * Gives in fix order, for after optimization. Adds parenthesis to show order of operation
     */
    string processInFix()
    {
        return root->processInFix();
    }
    /**
     * starts the optimization process to solve what we can
     */
    void optimize()
    {
        root->optimize();
    }
    /**
     * only optimize the 1*(something's) and (something's)*1
     */
    void optimizePar()
    {
        root->optimizePar();
    }
};
#endif