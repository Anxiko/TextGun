/*
 * TextGun.hpp
 *
 * Copyright 2016 Joaquín Monteagudo Gómez <kindos7@gmail.com>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston,
 * MA 02110-1301, USA.
 *
 *
 */

/*
    C++ library (header file)
    TextGun
    Node-Link frecuency based text learner
*/

/*
    Preprocessor
*/

/*Header guard*/
#ifndef _TEXT_GUN_H_
#define _TEXT_GUN_H_


/* Includes */

#include <list>//Linked lists
#include <utility>//Pairs
#include <map>//Maps
#include <string>//Strings
#include <istream>//Input stream
#include <sstream>//String stream
#include <random>//Random number generation
#include <chrono>
#include <ostream>//Writing to file
#include <istream>//Reading from file

/* Defines */

/*Constants*/

/*Macros*/

namespace TextGun
{
    /*
        Class definitions
    */

    enum class WordType : char;//Type of a word

    class Word;//Stores a word, indicates if it's special

    class FrecLink;//Array of links to nodes sorted based on their frecuencyclass FrecLink;//Array of links to nodes sorted based on their frecuency

    class WordNode;//Node for a word, frecuency and links on both directions

    class WordGraph;//Contains the WordNodes, indexed by their Word

    class TextStream;//Provides the Words from a input stream

    class WordModel;//Model capable of learning and speaking

    /*
        Function prototypes
    */

    /*
        Data types
     */

    /* Typedefs */

    /* Classes */

    //Type of a word
    enum class WordType : char
    {
        START=0,//Start of text
        WORD,//Text word
        END//End of text
    };

    //Stores a word, indicates if it's special
    class Word
    {
        /* Attributes */

        /*Word*/
        private:

            std::string s;//Word
            WordType t;//Type of the node

        /* Constructors, copy control */

        /*Constructors*/
        private:

            //Complete constructor
            Word(const std::string &ns,WordType nt);

        public:

            //Word constructor
            Word(const std::string &ns);

            //Special node constructor
            Word(WordType nt);


        /*Operator*/
        public:

            //Less than operator, needed for mapping
            bool operator<(const Word &w) const;

            //Equality operator
            bool operator==(const Word &w) const;

            //Inequality operator
            bool operator!=(const Word &w) const;

        /* Methods */

        /*Get/set*/
        public:

            //Get

            //Get the text
            const std::string& get_text() const
            {
                return s;
            }

            //Get the type
            WordType get_type() const
            {
                return t;
            }

            //Set

            //Set as a word
            void set_text(std::string ns)
            {
                s=ns;
                t=WordType::WORD;
            }

            //Set as a special word
            void set_type(WordType nt)
            {
                s="";
                t=nt;
            }

        /*Read/write to file*/
        public:

            //Write word to stream
            void write(std::ostream &o) const;

            //Read word to stream
            void read(std::istream &i);

    };

    //List of links to nodes sorted based on their frecuency
    class FrecLink
    {
        /* Config */

        /*Random*/
        private:

            //Random engine
            static std::default_random_engine re;

        /* Attributes */

        /*Links*/
        private:

            //Lists of words and their frecuency
            std::list< std::pair< int,Word > > words;

            //Dictionary that stores the position of each word on the list
            std::map< Word,std::list< std::pair< int,Word > >::iterator > dict;

            //Total number of words (sum of frec)
            int f;

            //Total number of links
            int n;

        /* Constructors, copy control */

        /*Constructors*/
        public:

            //Default constructors
            FrecLink()=default;

        /* Methods */

        /*Add/delete*/
        public:

            //Add a word to the list
            void add_word(const Word &w);

        private:

            //Take an iterator to a word on the list, and return another word to swap them so that the list is still sorted. Return the same iterator if no swap is needed
             std::list< std::pair< int,Word > >::iterator keep_sorted_swap(const std::list< std::pair< int,Word > >::iterator &it) const;

        /*Links*/
        public:

            //Get a random word based on frecuency
            Word get_rand() const;

        /*Read/write to file*/
        public:

            //Write word to stream
            void write(std::ostream &o) const;

            //Read word to stream
            void read(std::istream &i);
    };

    //Node for a word, frecuency and links on both directions
    class WordNode
    {
        /* Attributes */

        /*Links*/
        private:

            FrecLink prev,next;//Links to the wordnodes found before this one, and after this one

        /*Data*/
        public:

            //Data of this node

            Word w;//Word stored on this node
            int f;//Frecuency of this word

        /* Constructors, copy control */

        /*Constructors*/
        public:

            //Complete constructor
            WordNode(const Word &nw);

        /* Methods */

        /*Links*/
        public:

            //Add a link

            //Add a link to a previous word
            void add_prev(const Word &w);

            //Add a link to a next word
            void add_next(const Word &w);

            //Get a random word

            //Get a random previous word
            Word get_prev() const;

            //Get a random next word
            Word get_next() const;

        /*Word*/
        public:

            //Increase frecuency
            void inc_frec();

            //Get word
            const Word& get_word() const
            {
                return w;
            }

        /*Read/write to file*/
        public:

            //Write to file
            void write(std::ostream &o) const;

            //Read from file
            void read(std::istream &i);

    };


    //Contains the WordNodes, indexed by their Word
    class WordGraph
    {
        /* Attributes */

        /*Nodes*/
        private:

            std::map< Word,WordNode > nodes;//Nodes indexed by their word
            int n;//Number of nodes

        /* Constructors, copy control */
        public:

            //Default constructor
            WordGraph()=default;

        /* Methods */

        /*Nodes*/
        public:

            //Check if a word exists (as a node in the graph)
            bool check_word(const Word &w);

            //Add a word to the node, increase its frecuency if it exists
            void add_word(const Word &w);

            //Get a node by pointer, nullptr if not found
            WordNode* get_node(const Word &w);

        /*Links*/
        public:

            //Add a link between two nodes
            void add_link(const Word &prev, const Word &next);

        /*Read/write to file*/
        public:

            //Write to file
            void write(std::ostream &o) const;

            //Read from file
            void read(std::istream &i);
    };

    //Provides the Words from a input stream
    class TextStream
    {
        /* Config */

        /*Types*/
        private:

            enum class StreamState
            {
                START,//Feed start word
                TEXT,//Feed text words from the stream
                END,//Feed end word
                EMPTY//Stream is empty, will not provide more words
            };

        /*Defaults*/
        private:

            //Word to be returned if an error arises during reading
            static const Word DEF_ERR_WORD;

        /* Attributes */

        /*Stream*/
        private:

            //Input stream
            std::istream &is;

            //Status of the stream
            StreamState status;

            //Next word to be fed
            Word nw;

        /* Constructors, copy control */

        /*Constructors*/
        public:

            //Complete constructor
            TextStream(std::istream &nis);

        /* Methods */

        /*Stream*/
        public:

            //Loads the next words from the stream, returns if the stream is ready. Must be called after every read
            bool has_words();

            //Return the last word read from the stream
            Word read();

    };

    //Model capable of learning and speaking
    class WordModel
    {
        /* Attributes */

        /*Nodes*/
        private:

            WordGraph graph;//Graph to be trained and to generate sentences

        /* Constructors, copy control */

        /*Constructors*/
        public:

            WordModel()=default;

        /* Methods */

        /*Learn*/
        public:

            //Learn from a text stream
            void learn(TextStream &ts);

        /*Speak*/
        public:

            //Generate a line using the model
            std::string think();
    };

}//End of namespace

//End of library
#endif // _TEXT_GUN_H_
