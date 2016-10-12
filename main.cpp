//TextGun model
#include "TextGun.hpp"

#include <string>//Strings

#include <fstream>//File stream

#include <sstream>//String stream

#include <iostream>//Print to console

//File to open
const std::string FILE_NAME("TextLearning.txt");

int main()
{
    TextGun::WordModel model;//Model to be trained
    std::ifstream txt_stream(FILE_NAME);//Open the file with the text
    std::string buffer;//Hold the last read line
    while(std::getline(txt_stream,buffer))//Load a line from the file
    {
        std::stringstream ss(buffer);//Create a string stream
        TextGun::TextStream ts(ss);//Create a text stream
        model.learn(ts);//Learn from this line
    }

    std::cout<<model.think();

    return 0;
}
