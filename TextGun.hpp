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

/* Defines */

/*Constants*/

/*Macros*/

namespace TextGun
{
    /*
        Class definitions
    */

    enum class WordType;//Type of a word

    class Word;//Stores a word, indicates if it's special

    class FrecLink;//Array of links to nodes sorted based on their frecuencyclass FrecLink;//Array of links to nodes sorted based on their frecuency

    class WordNode;//Node for a word, frecuency and links on both directions

    /*
        Function prototypes
    */

    /*
        Data types
     */

    /* Typedefs */

    /* Classes */

    //Type of a word
    enum class WordType
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

        /* Methods */

        /*Get/set*/
        public:

            //Get

            //Get the text
            const std::string& get_text()
            {
                return s;
            }

            //Get the type
            WordType get_type()
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

    };

    //List of links to nodes sorted based on their frecuency
    class FrecLink
    {
        /* Attributes */

        /*Links*/
        private:

            //Lists of words and their frecuency
            std::list< std::pair< int,Word > > words;

            //Dictionary that stores the position of each word on the list
            std::map< Word,std::list< std::pair< int,Word > >::iterator > dict;

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

            //Take an iterator to an word on the list, and return another word to swap them so that the list is still sorted. Return the same iterator if no swap is needed
             std::list< std::pair< int,Word > >::iterator keep_sorted_swap(const std::list< std::pair< int,Word > >::iterator &it);
    };

    //Node for a word, frecuency and links on both directions
    class WordNode
    {
        /* Attributes */

        /*Links*/
        private:

            //Links to the wordnodes found before this one, and after this one

            FrecLink prev,next;//List of nodes
            int n_prev,n_next;//Total frecuency of links

        /*Data*/
        public:

            //Data of this node

            Word w;//Word stored on this node
            int f;//Frecuency of this word

        /* Constructors, copy control */

        /*Constructors*/
        public:

            //Default constructors
            WordNode()=default;


    };
}//End of namespace

//End of library
#endif // _TEXT_GUN_H_
