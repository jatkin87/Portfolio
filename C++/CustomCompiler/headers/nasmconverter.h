#ifndef NASMCONVERTER_H
#define NASMCONVERTER_H
#include <fstream>
#include <istream>

/**
 * holds the file that is being saved to, and helper functions to add nasm commands
 */
class NASMConverter
{
private:
    ofstream file; //file to save to
    string filename; //name of the file
public:
    /**
     * on complete, add in the footer to the file and close it
     */
    ~NASMConverter() 
    { 
        file << "\ndone:\n";
        file << "mov     rbx, 0\n";
        file << "mov     rax, 1\n";
        file << "int 0x80\n";

        file.close(); 
    }
    /**
     * this starts the file process, will add the header to the asm file
     * 
     * @param filename name of the file, without extension
     * @param debug if we want to print comments into the nasm file
     */
    NASMConverter(string filename = "output", bool debug = true)
    {
        this->filename = filename;
        file.open(filename + ".asm"); //open the file
        void* run = debug ? addHeader() : nullptr; //if debug, print how to run info as a comment on top

        //asm section .data
        file << "section .data\n";
        file << "int32:     db \"%d\", 10, 0\n";
        file << "float32:   db \"%f\", 10, 0\n\n";

        //asm section .text
        file << "section .text\n";
        file << "global main\n";
        file << "extern printf\n\n";
    }
    /**
     * write to file
     * 
     * @param s address of the string to write to the file
     */
    void * writeString(string &s)
    {
        file << s;
        return nullptr;
    }
    /**
     * returns the string representation of an add in nasm
     * 
     * @param loc location to store the result in
     * @param a a in a + b
     * @param b b in a + b
     * @return string representation of add in nasm
     */
    string Add(string loc, string a, string b)
    {
        string f = "";
        f += "mov eax, " + a + "\n";
        f += "add eax, " + b + "\n";
        f += "mov " + loc + ", eax\n\n";
        return f;
    }
    /**
     * returns the string representation of an subtract in nasm
     * 
     * @param loc location to store the result in
     * @param a a in a - b
     * @param b b in a - b
     * @return string representation of subtract in nasm
     */
    string Sub(string loc, string a, string b)
    {
        string f = "";
        f += "mov eax, " + a + "\n";
        f += "sub eax, " + b + "\n";
        f += "mov " + loc + ", eax\n\n";
        return f;
    }
    /**
     * string representation of the divide command in nasm
     * 
     * @param loc location to store the information
     * @param num numerator
     * @param den denomonator
     * @return string representation of the divide command in nasm
     * @throws divide by 0 error
     */
    string Divide(string loc, string num, string den)
    {
        if(den == "0")
        {
            throw runtime_error("Denomonator cannot be 0");
        }
        string f = "";
        f += "mov edx, 0\n";
        f += "mov eax, " + num + "\n";
        f += "mov ecx, " + den + "\n";
        f += "div ecx\n";
        f += "mov " + loc + ", ecx\n\n";
        return f;
    }
    /**
     * returns the string representation of a multiply in nasm
     * 
     * @param loc location to store the result in
     * @param a a in a * b
     * @param b b in a * b
     * @return string representation of multiply in nasm
     */
    string Mul(string loc, string a, string b)
    {
        string f = "";
        f += "mov eax, " + a + "\n";
        if(isdigit(b[0]))
        {
            f += "imul eax, eax, " + b + "\n"; 
        }
        else
        {
            f += "mul " + b + "\n";
        }
        f += "mov " + loc + ", eax\n\n";
        return f;
    }
    /**
     * the print command
     * 
     * this will change when I add functions because I am going to treat print like a function
     */
    string Print(string loc, string format)
    {
        string f = "";
        f += "mov edi, " + format + "\n";
        f += "mov esi, " + loc + "\n";
        f += "mov eax, 0\n";      
        f += "call printf\n\n";
        return f;
    }
    /**
     * string representation of the move command in nasm
     * 
     * @param value to move
     * @param loc location to move to
     * 
     * @return string representation of the move command in nasm
     */
    string Mov(string value, string loc)
    {
        // if both value and location are stack locations, call Add to 0 instead
        return !((isdigit(value[value.size()-1])) || (loc.at(loc.size()-1) == 'x') || (value.at(value.size()-1) == 'x') ) ? Add(loc, "0", value) : "mov " + loc + ", " + value + "\n\n";
    }
    /**
     * returns the header for a stack space, to declare stack space
     * 
     * @param size string representation of the size to ask for
     * @return string representation of rsp stack allocation
     */
    string StackSize(string size)
    {
        string f = "";
        f += "push rbp\n";
        f += "mov rbp, rsp\n";
        f += "sub rsp, " + size + "\n";
        return f;
    }
    /**
     * comparison between two values
     * 
     * @param a value 1 to compare
     * @param b value 2 to compare
     * @return a string representation of the compare expression
     */
    string Compare(string a, string b)
    {
        string f = "";
        f += Mov(a, "eax");
        f += Mov(b, "ebx");
        f += "cmp eax, ebx\n";
        return f;
    }
    /**
     * checking if two values are equal. Equal falls through, not equal jumps
     * 
     * @param a var 1
     * @param b var 2
     * @param scopeName name of the label to jump to if NOT EQUAL
     * @return string representation of an equality statement
     */
    string isEqual(string a, string b, string scopeName)
    {
        string f = "";
        f += Compare(a, b);
        f += "jne ." + scopeName + "\n";
        return f;
    }
    /**
     * check if less than. If less than, fall through, else jump to false label
     * 
     * @param a var 1
     * @param b var 2
     * @param scopeName false expression label
     * @return string representation of a less than statement
     */
    string lessThan(string a, string b, string scopeName)
    {
        string f = "";
        f += Compare(a, b);
        f += "jge ." + scopeName + "\n";
        return f;
    }
    /**
     * Power loop
     * 
     * @param base base number
     * @param expo exponent
     * @param loc register to store the answer into
     * @param line line number, for labeling
     * @return a string representation of a power loop
     */
    string Pow(string base, string expo, string loc, string line)
    {
        string f = "";
        f += "pow_" + line + ":\n";
        f += "mov ecx, 0\n";
        f += "mov eax, 1\n";
        f += "cmp ecx, " + expo + "\n";
        f += "je powdone_" + line + "\n";
        f += "powloop_" + line + ":\n";
        f += "mul " + base + "\n";
        f += "inc ecx\n";
        f += "cmp ecx, " + expo + "\n";
        f += "jne powloop_" + line + "\n";
        f += "powdone_" + line + ":\n";
        f += Mov("eax", loc);
        return f;
    }
    /**
     * debug header
     */
    void* addHeader()
    {
        file << "; How to build\n";
        file << "; nasm -f elf64 -o " << filename << ".o -l " << filename << ".lst " << filename << ".asm\n";
        file << "; gcc -no-pie -o " << filename << " " <<  filename << ".o\n";
        file << "; ./" << filename << "\n\n";
        return nullptr;
    }

};

#endif