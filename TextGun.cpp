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
        Init
    */

    /* FrecLink */

    std::default_random_engine FrecLink::re(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));//Random engine

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

    //Equality operator
    bool Word::operator==(const Word &w) const
    {
        return (t==w.t)&&(s==w.s);
    }

    //Inequality operator
    bool Word::operator!=(const Word &w) const
    {
        return !operator==(w);
    }

    /* Methods */

    /*Read/write to file*/

    //Write word to stream
    void Word::write(std::ostream &o) const
    {
        //Write the type of the word
        o.write(reinterpret_cast<const char *>(&t),sizeof(char));

        //Write the number of bytes of the string
        std::string::size_type sz=s.size();
        o.write(reinterpret_cast<const char *>(&sz),sizeof(int));

        //Write the string
        o.write(s.c_str(),sz);
    }

    //Read word to stream
    void Word::read(std::istream &i)
    {
        //Read the type of the word
        i.read(reinterpret_cast<char *>(&t),sizeof(char));

        //Read the number of bytes
        std::string::size_type sz;
        i.read(reinterpret_cast<char *>(&sz),sizeof(std::string::size_type));

        //Create a buffer
        char *str=new char[sz+1];

        //Read to buffer
        i.read(str,sz);
        str[sz]='\0';//Ensure null terminated

        //Put into string
        s=std::string(str);

        //Delete buffer
        delete[] str;
    }

    /*
        FrecLink
    */

    /* Methods */

    /*Add/delete*/

    //Add a word to the list
    void FrecLink::add_word(const Word &w)
    {
        ++f;//Increase the frecuency of total links

        //Check if the word is on the list
        if (dict.find(w)==dict.end())//Not found
        {
            //Insert it at the end of the list with frec=1 (since it's the first time this word's been seen)
            words.emplace_back(1,w);
            //Add the word to the dictionary
            dict[w]=std::prev(words.end());//Save the iterator to the last element of the list, where the word is now saved

            ++n;//A new node on the list
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

    /*Links*/

    //Get a random word based on frecuency
    Word FrecLink::get_rand() const
    {
        //Create the RNG to use it with the engine
        std::uniform_int_distribution<> dt(0,f-1);

        //Random number generated
        int n=dt(re);

        //Navigate through the links until the goal number is met
        for (std::pair< int,Word > w : words)
        {
            n-=w.first;//Decrease the goal by this word's frecuency

            if(n<0)//If the goal is met, return this word
                return w.second;
        }

        //If no word is found, an error just happened
        return Word(WordType::END);//Should throw an exception, by the time being the END world will be returned. Note that the word END is valid
    }

    /*Read/write to file*/

    //Write word to stream
    void FrecLink::write(std::ostream &o) const
    {
        //Write the number of entries
        o.write(reinterpret_cast<const char *>(&n),sizeof(int));
        //Write the sum of the frecuencies
        o.write(reinterpret_cast<const char *>(&f),sizeof(int));

        //Write the list
        for(const std::pair< int,Word > &p : words)
        {
            //Write the frec
            o.write(reinterpret_cast<const char *>(&p.first),sizeof(int));

            //Write the word
            p.second.write(o);
        }
    }

    //Read word to stream
    void FrecLink::read(std::istream &i)
    {
        //Read number of entries
        n=0;//Set to zero in case of fail reading
        i.read(reinterpret_cast<char *>(&n),sizeof(int));
        //Read the sum of the frecuencies
        i.read(reinterpret_cast<char *>(&f),sizeof(int));

        //Iterator to insert
        auto it=words.begin()==words.end()?words.begin():std::prev(words.end());//This list should always be empty, just to make sure, start at the end

        //Read the list
        int iters=n;
        while(iters-->0)//Read all the entries
        {
            //Read the frec
            int word_frec=0;
            i.read(reinterpret_cast<char *>(&word_frec),sizeof(int));

            //Read the word
            Word w("");
            w.read(i);

            //Insert the word on the list
            it=words.emplace(it,word_frec,w);

            //Insert the word on the map
            dict[w]=it++;//Increment the iterator after inserting, as to keep inserting after the end
        }
    }

    /*
        WordNode
    */

    /* Constructors, copy control */

    /*Constructors*/

    //Complete constructor
    WordNode::WordNode(const Word &nw)
    :prev(),next(),w(nw),f(1)
    {}

    /* Methods */

    /*Links*/

    //Add a link

    //Add a link to a previous word
    void WordNode::add_prev(const Word &w)
    {
        prev.add_word(w);
    }

    //Add a link to a next word
    void WordNode::add_next(const Word &w)
    {
        next.add_word(w);
    }

    //Get a random word

    //Get a random previous word
    Word WordNode::get_prev() const
    {
        return prev.get_rand();
    }

    //Get a random next word
    Word WordNode::get_next() const
    {
        return next.get_rand();
    }

    /*Word*/

    //Increase frecuency
    void WordNode::inc_frec()
    {
        ++f;
    }

    /*Read/write to file*/

    //Write to file
    void WordNode::write(std::ostream &o) const
    {
        //Write the word of the node
        w.write(o);

        //Write the frecuency
        o.write(reinterpret_cast<const char *>(&f),sizeof(int));

        //Write the links to previous words
        prev.write(o);

        //Write the links to next words
        next.write(o);
    }

    //Read from file
    void WordNode::read(std::istream &i)
    {
        //Read the word of the node
        w.read(i);

        //Read the frecuency
        i.read(reinterpret_cast<char *>(&f),sizeof(int));

        //Read the links to previous words
        prev.read(i);

        //Read the links to next words
        next.read(i);
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
        {
            nodes.emplace(w,w);
            ++n;
        }
        else//If found, increase
            it->second.inc_frec();
    }

    //Get a node by pointer, nullptr if not found
    WordNode* WordGraph::get_node(const Word &w)
    {
        if(check_word(w))
        {
            WordNode &node=nodes.at(w);
            return &node;
        }
        return nullptr;
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

    /*Read/write to file*/

    //Write to file
    void WordGraph::write(std::ostream &o) const
    {
        //Write the number of words
        o.write(reinterpret_cast<const char *>(&n),sizeof(int));

        //Write all the nodes
        for(const std::pair< const Word, const WordNode > &n : nodes)
            n.second.write(o);
    }

    //Read from file
    void WordGraph::read(std::istream &i)
    {
        //Read the number of words
        n=0;
        i.read(reinterpret_cast<char *>(&n),sizeof(int));

        //Read the words
        int iters=n;
        while(iters-->0)//Read all the words
        {
            //Node to read
            WordNode wn(Word(""));

            //Read the node
            wn.read(i);

            //Insert the node on the map, indexed by its word
            nodes.insert(std::pair< Word,WordNode >(wn.get_word(),wn));
        }
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
                if (is.good())//Try to read
                {
                    is >> s;
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

    /*Speak*/

    //Generate a line using the model
    std::string WordModel::think()
    {
        std::stringstream ss;//Stream to write the line built by the model

        //Make sure the start and end node exist

        if (!graph.check_word(Word(WordType::START)))
            graph.add_word(Word(WordType::START));

        if (!graph.check_word(Word(WordType::END)))
            graph.add_word(Word(WordType::END));

        WordNode *node=graph.get_node(Word(WordType::START));//The first node to be processed is the start node

        const Word end_word(WordType::END);


        while(node&&node->get_word()!=end_word)//Until the end node is reached
        {
            ss<<node->get_word().get_text()<<' ';//Print this node

            //Advance to next
            node=graph.get_node(node->get_next());

        }

        return ss.str();
    }

}//End of namespace
