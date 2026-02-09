#ifndef COLORS_H
#define COLORS_H
#include <string>
#include <vector>
#include <map>
using namespace std;


/**
 * A helper class that returns escape sequence for coloring text in a Linux console
 */
class Colors
{
private:
    //A vector holding all the names for the valid colors
    vector<string> colorNames
    {
        "Black",
        "Red",
        "Green",
        "Yellow",
        "Blue",
        "Magenta",
        "Cyan",
        "White"
    };
    //A map that associates the color name with it's escape sequence
    map<string,string> colors;
    pair<string,string> underline{"\033[4m", "\033[24m"};

public:
    /**
     * Default constructor creates the escape sequence for each color
     */
    Colors()
    {
        for(int i = 0; i < colorNames.size(); i++)
        {
            #ifdef linux
            colors[colorNames[i]] = "\x1B[" + to_string(i+30) + "m" ;
            #endif
        }
    }
    /**
     * Gets the escape sequence for the corresponding color, no error checking.
     * use printValidColorNames to see list of valid keys
     * 
     * @param name color name, starting with a capital letter i.e. RENAME_WHITEOUT
     * @return escape sequence for the corresponding color
     */
    string getColor(string name)
    {
        return colors[name];
    }
    /**
     * returns a string that combines the input string with the escape sequence, and
     * a return to the original color.
     * No error checking, use printValidColorNames() to see valid keys
     * 
     * @param colorName name of the color you want the text to be
     * @param s the text to be colored
     * @param returnColor the color to return to so nothing else prints the color, defaults to White
     * 
     * @return a string that is encapsulated by the color, and the return color
     */
    string printInColor(string colorName, string s, string returnColor = "White")
    {
        return colors[colorName] + s + colors[returnColor];
    }
    /**
     * underline the text
     * 
     * @param text string to underliine
     * @return text that is encapsulated by the underline escape sequence
     */
    string Underline(string text)
    {
        return underline.first + text + underline.second;
    }
    /**
     * This prints all valid color keys
     */
    void printValidColorNames()
    {
        for(auto &s : colorNames)
        {
            printf("%s\n", s.c_str());
        }
    }
    /**
     * inverses the foreground and background
     * 
     * @param text text to inverse
     * @return text encapsulated with the reverse escape sequence
     */
    string inverseColors(string text)
    {
        return "\033[0;7m" + text + "\033[0;0m";
    }
};
#endif