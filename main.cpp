//TextGun model
#include "TextGun.hpp"

#include <string>//Strings

#include <fstream>//File stream

#include <sstream>//String stream

#include <iostream>//Print to console

#include <exception>//Exception handling

//Number of options
enum Options: int
{
    START=0,//Start of valid options
    LISTEN,//Read a single line from standard input
    THINK,//Produce output
    READ,//Read model from file
    WRITE,//Write the model to file
    LEARN,//Learn model from file
    EXIT,//Exit the program
    ERROR,//Invalid option
    END//End of valid values
};

//Print menu, and return picked option
Options menu();

//Make a yes/no question
bool questYN(const std::string &s, bool def);

//Read a word containing the name of a file on the current path
std::string read_filename();

//Read a complete line, except the new line character
std::string read_line();

int main()
{
    //Flags to control program flow
    bool in=true;//Stay in the program loop

    bool unsaved_changes=false;//Changes to be saved
    bool empty_model=true;//Model is completly empty

    //Model
    TextGun::WordModel model;//TextGun model

    while(in)
    {
            switch(menu())
            {
                //Learn a line from standard input
                case Options::LISTEN:
                {
                    //Read the line
                    std::cout<<"Input line to learn: ";
                    std::string s=read_line();

                    //Feed the line to the model
                    std::stringstream ss(s);
                    TextGun::TextStream ts(ss);
                    model.learn(ts);

                    //Adjust the flags
                    unsaved_changes=true;
                    empty_model=false;

                    break;
                }

                //Generate and write lines
                case Options::THINK:
                {
                    //Print lines until users requests to stop
                    do
                    {
                        std::cout<<model.think()<<'\n';
                    }while(!questYN("Quit?",false));
                    break;
                }

                //Read a pregenerated model from a binary file
                case Options::READ:
                {
                    if (!empty_model)
                        std::cout<<"ERROR! Cannot read from file because model isn't empty. Reopen the program, and read the file\n";
                    else
                    {

                        //Path of file to read
                        std::cout<<"File to read: ";
                        std::string file=read_filename();

                        //Try to open the file
                        std::ifstream input(file,std::ios::in|std::ios::binary);
                        if(input.is_open())//If the file is open, save
                        {
                            model.read(input);
                            empty_model=false;
                        }
                        else
                            std::cout<<"ERROR: reading from file "<<file<<'\n';
                    }

                    break;
                }

                //Write the current model to file
                case Options::WRITE:
                {
                    //Path of file to write
                    std::cout<<"File to write: ";
                    std::string file=read_filename();

                    //Try to open the file
                    std::ofstream output(file,std::ios::out|std::ios::binary|std::ios::trunc);
                    if(output.is_open())//If the file is open, write
                    {
                        if (unsaved_changes)//If there are unsaved changes, go ahead
                        {
                            model.write(output);
                            unsaved_changes=false;
                        }
                        else//If there are no changes, no need to save
                            std::cout<<"No need to save anything!\n";
                    }
                    else
                        std::cout<<"ERROR: saving to file "<<file<<'\n';

                    break;
                }

                case Options::LEARN:
                {
                    //Path of file to open
                    std::cout<<"File to learn from: ";
                    std::string file=read_filename();

                    //Try to open the file
                    std::ifstream input(file,std::ios::in|std::ios::binary);
                    if(input.is_open())//If the file is open, read it
                    {
                        //Read the file, line by line
                        for(std::string s;std::getline(input,s);)
                        {
                            if (!s.empty())//Don't read blank lines
                            {
                                std::stringstream ss(s);
                                TextGun::TextStream ts(ss);
                                model.learn(ts);

                                //Modify flags
                                unsaved_changes=true;
                                empty_model=false;
                            }
                        }
                    }
                    else
                        std::cout<<"ERROR! Reading from file "<<file<<'\n';

                    break;
                }

                //Exit the program
                case Options::EXIT:
                {
                    if(questYN("Close the program?",false))
                        in=false;

                    break;
                }

                case Options::ERROR:
                default:
                {
                    std::cout<<"ERROR!\n";
                    break;
                }
            }
	}

	return 0;
}

//Print menu, and return picked option
Options menu()
{
    std::cout<<"Menu\n";

    std::cout<<'['<<Options::LISTEN<<']'<<" Listen - read and learn one line from console input\n";
    std::cout<<'['<<Options::THINK<<']'<<" Think - generate output\n";
    std::cout<<'['<<Options::READ<<']'<<" Read - read a binary file of a previously saved model\n";
    std::cout<<'['<<Options::WRITE<<']'<<" Write - write current model to binary format\n";
    std::cout<<'['<<Options::LEARN<<']'<<" Learn - generate a model from a text file (one entry per new line, no empty lines)\n";
    std::cout<<'['<<Options::EXIT<<']'<<" Exit - leave the program\n";
    std::cout<<"Option => ";

    std::string opt;
    std::getline(std::cin,opt);
    int x;

    //Parse the string to int, errors may occur
    std::stringstream ss(opt);
    if(!(ss>>x))
        x=Options::ERROR;

    if (x<=Options::START||x>=Options::END)
        x=Options::ERROR;

    return static_cast<Options>(x);
}

//Make a yes/no question
bool questYN(const std::string &s, bool def)
{
    for(;;)
    {
        std::cout<<s<<(def?"[Y/n]":"[y/N]")<<": ";
        std::string s;
        std::getline(std::cin,s);

        if (s=="y"||s=="Y")
            return true;
        if (s=="n"||s=="N")
            return false;
        if (s=="")
            return def;
        std::cout<<"ERROR: Invalid option. Please use y,Y,n,N or a blank line for default value\n";
    }
}


//Read a word containing the name of a file on the current path
std::string read_filename()
{
    std::string s;
    std::getline(std::cin,s);

    return std::move(s);
}

//Read a complete line, except the new line character
std::string read_line()
{
    std::string s;
    std::getline(std::cin,s);

    return std::move(s);
}
