/*
 * TextGun.cpp
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
    C++ library (source file)
    TextGun
    Node-Link frecuency based text learner
*/

/*
    Preprocessor
*/

/* Includes */

//Header file
#include "TextGun.hpp"

namespace TextGun
{
    /*
            Functions
    */

    /*
        Word
    */

    /* Constructors, copy control */

    /*Constructors*/

    //Complete constructor
    Word::Word(const std::string &ns,WordType nt)
    :s(ns),t(nt)
    {}

    //Word constructor
    Word::Word(const std::string &ns)
    :Word(ns,WordType::WORD)
    {}

    //Special node constructor
    Word::Word(WordType nt)
    :Word("",nt)
    {}

    /*Operator*/

    //Less than operator, needed for mapping
    bool Word::operator<(const Word &w) const
    {
        //Compare first by type, then by text

        if (t!=w.t)
            return t<w.t;
        return s<w.s;
    }

    /*
        FrecLink
    */

    /* Methods */

    /*Add/delete*/

    //Add a word to the list
    void FrecLink::add_word(const Word &w)
    {
        //Check if the word is on the list
        if (dict.find(w)==dict.end())//Not found
        {
            //Insert it at the end of the list with frec=1 (since it's the first time this word's been seen)
            words.emplace_back(1,w);
            //Add the word to the dictionary
            dict[w]=std::prev(words.end());//Save the iterator to the last element of the list, where the word is now saved
        }
        else//Found
        {
            std::list< std::pair< int,Word > >::iterator pos=dict[w];//Get the position of the pair
            ++(pos->first);//Increment the frecuency by one

            //Find the new position on the list for this element
            std::list< std::pair< int,Word > >::iterator new_pos=keep_sorted_swap(pos);

            //Swap positions, only if they're different
            if (pos!=new_pos) words.splice(new_pos,words,pos);
        }
    }

    //Take an iterator to a word on the list, and return another word to swap them so that the list is still sorted. Return the same iterator if no swap is needed
    std::list< std::pair< int,Word > >::iterator FrecLink::keep_sorted_swap(const std::list< std::pair< int,Word > >::iterator &pos) const
    {
        std::reverse_iterator< std::list< std::pair< int,Word > >::iterator > rev_it(pos);//Reverse operator to start looking at one upper from this one
        /*
          std::advance(rev_it,1);//Advance the iterator by one, no longer looking at the same element, but at the upper one. Iterator's valid because pos!=begin()
          No need to do this! Given the way reverse iterators work, rev_it is using pos as its base(), so it's alredy pointing at the next value
        */

        while (rev_it!=words.rend())//Look while there are still elemets to iterate
        {
            if((rev_it->first) >= (pos->first))//Find one to switch for
                break;//Stop the search
            ++rev_it;//If nothing was found, continue searching
        }

        /*
            Swap the position between pos and the new found one (rev_it).
            Get the forward iterator, which points to the previous element, using base().
            If no value was found, rev_it==rend(), which means rev_it.base()==begin(),
            so it'll be swapped to the top where it belongs (since no value being found means this is the biggest one).
            If a value was found, then base() contains the previous value,
            so it'll be swapped right before the value found, keeping things sorted.
            NOTE: getting base() from a reverse iterator will always be valid if the reverse iterator was valid,
            keeping in mind that base() of rbegin() is end(), which is a valid iterator that cannot be dereferenced.
        */

        return rev_it.base();
    }

    /*
        WordNode
    */

    /* Constructors, copy control */

    /*Constructors*/

    //Complete constructor
    WordNode::WordNode(const Word &nw)
    :prev(),next(),n_prev(0),n_next(0),w(nw),f(1)
    {}

    /* Methods */

    /*Links*/

    //Add a link

    //Add a link to a previous word
    void WordNode::add_prev(const Word &w)
    {
        prev.add_word(w);
        ++n_prev;
    }

    //Add a link to a next word
    void WordNode::add_next(const Word &w)
    {
        next.add_word(w);
        ++n_next;
    }

    /*Word*/

    //Increase frecuency
    void WordNode::inc_frec()
    {
        ++f;
    }

    /*
        WordGraph
    */

    /* Methods */

    /*Nodes*/

    //Check if a word exists (as a node in the graph)
    bool WordGraph::check_word(const Word &w)
    {
        return nodes.find(w)!=nodes.end();
    }

    //Add a word to the node, increase its frecuency if it exists
    void WordGraph::add_word(const Word &w)
    {
        auto it=nodes.find(w);

        if (it==nodes.end())//Add if not found
            nodes.emplace(w,w);
        else//If found, increase
            it->second.inc_frec();
    }

    /*Links*/

    //Add a link between two nodes
    void WordGraph::add_link(const Word &prev, const Word &next)
    {
        //Assuming both nodes alredy exist

        //Add link prev -> next
        nodes.at(prev).add_next(next);
        nodes.at(next).add_prev(prev);
    }

    /*
        TextStream
    */

    /* Constructors, copy control */

    /*Constructors*/

    //Complete constructor
    TextStream::TextStream(std::istream &nis)
    :is(nis),status(StreamState::START),nw("")
    {}

    /* Methods */

    /*Stream*/

    //Loads the next words from the stream, returns if the stream is ready. Must be called after every read
    bool TextStream::has_words()
    {
        switch(status)
        {
            //Load the start word, switch to text
            case StreamState::START:
            {
                nw=Word(WordType::START);
                status=StreamState::TEXT;
                return true;
            }

            //Load a text word, if it fails, switch to the end
            case StreamState::TEXT:
            {
                std::string s;//String to be read
                if ((is>>s).good())//Try to read
                {
                    nw=Word(s);
                    return true;
                }
                else
                {
                    //If the word could not be read, let the flow reach to the next case. Do not break or return
                    status=StreamState::END;
                }
            }

            //End of text, stream is now empty
            case StreamState::END:
            {
                nw=Word(WordType::END);
                status=StreamState::EMPTY;
                return true;
            }

            //When the stream is empty or on a invalid state, it has no word
            case StreamState::EMPTY:
            default:
                return false;
        }
    }

    //Return the last word read from the stream
    Word TextStream::read()
    {
        return nw;
    }

    /*
        WordModel
    */

    /* Methods */

    /*Learn*/

    //Learn from a text stream
    void WordModel::learn(TextStream &ts)
    {
        //Load the first word
        if (ts.has_words())
        {
            Word w=ts.read();//Word to be processed
            graph.add_word(w);//Add it to the graph

            //Read the rest on a loop
            Word prev=w;//Previous word to be processed

            //While the stream has words
            while (ts.has_words())
            {
                //Read the word
                w=ts.read();

                //Add the word
                graph.add_word(w);

                //Add the links
                graph.add_link(prev,w);

                //Store the current word on previous
                prev=w;
            }
        }
    }

}//End of namespace
