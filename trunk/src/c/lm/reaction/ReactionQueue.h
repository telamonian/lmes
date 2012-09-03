/*
 * University of Illinois Open Source License
 * Copyright 2011 Luthey-Schulten Group,
 * All rights reserved.
 * 
 * Developed by: Luthey-Schulten Group
 * 			     University of Illinois at Urbana-Champaign
 * 			     http://www.scs.uiuc.edu/~schulten
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the Software), to deal with 
 * the Software without restriction, including without limitation the rights to 
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies 
 * of the Software, and to permit persons to whom the Software is furnished to 
 * do so, subject to the following conditions:
 * 
 * - Redistributions of source code must retain the above copyright notice, 
 * this list of conditions and the following disclaimers.
 * 
 * - Redistributions in binary form must reproduce the above copyright notice, 
 * this list of conditions and the following disclaimers in the documentation 
 * and/or other materials provided with the distribution.
 * 
 * - Neither the names of the Luthey-Schulten Group, University of Illinois at
 * Urbana-Champaign, nor the names of its contributors may be used to endorse or
 * promote products derived from this Software without specific prior written
 * permission.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR 
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, 
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL 
 * THE CONTRIBUTORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR 
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, 
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR 
 * OTHER DEALINGS WITH THE SOFTWARE.
 *
 * Author(s): Elijah Roberts
 */

#ifndef LM_REACTION_REACTIONQUEUE_H_
#define LM_REACTION_REACTIONQUEUE_H_

#include <cmath>

namespace lm {
namespace reaction {

class ReactionQueue
{
public:
    struct ReactionEvent
    {
        double time;
        double propensity;
    };

public:
    ReactionQueue(uint numberReactions):numberReactions(numberReactions),reactionEvents(NULL),reactionQueue(NULL),reactionPositions(NULL)
    {
        reactionEvents = new ReactionEvent[numberReactions];
        reactionQueue = new uint[numberReactions+1];
        reactionPositions = new uint[numberReactions];
        reactionQueue[0] = 0;
        for (uint i=0; i<numberReactions; i++)
        {
            reactionEvents[i].time = INFINITY;
            reactionEvents[i].propensity = 0.0;
            reactionQueue[i+1] = i;
            reactionPositions[i] = i+1;
        }
    }

    ~ReactionQueue()
    {
        if (reactionEvents != NULL) delete[] reactionEvents; reactionEvents = NULL;
        if (reactionQueue != NULL) delete[] reactionQueue; reactionQueue = NULL;
        if (reactionPositions != NULL) delete[] reactionPositions; reactionPositions = NULL;
    }

    uint getNextReaction()
    {
        return reactionQueue[1];
    }

    ReactionEvent getReactionEvent(uint reactionIndex)
    {
        return reactionEvents[reactionIndex];
    }

    void updateReactionEvent(uint reactionIndex, double newTime, double newPropensity)
    {
        // Update the reaction entry.
        double oldTime = reactionEvents[reactionIndex].time;
        reactionEvents[reactionIndex].time = newTime;
        reactionEvents[reactionIndex].propensity = newPropensity;

        // Get the reaction's current position in the queue.
        uint currentPosition = reactionPositions[reactionIndex];

        // If the new time is before the old time we need to move up in the tree.
        if (newTime <= oldTime)
        {
            // While the new time is less that its parent's time, move it up.
            uint parentPosition = currentPosition>>1;
            uint parentReactionIndex = reactionQueue[parentPosition];
            while (parentPosition >= 1 && newTime < reactionEvents[parentReactionIndex].time)
            {
                // Swap the reaction with its parent.
                reactionQueue[parentPosition] = reactionIndex;
                reactionQueue[currentPosition] = parentReactionIndex;

                // Update the position list.
                reactionPositions[reactionIndex] = parentPosition;
                reactionPositions[parentReactionIndex] = currentPosition;

                // Set our new current position and loop again.
                currentPosition = parentPosition;
                parentPosition = currentPosition>>1;
                parentReactionIndex = reactionQueue[parentPosition];
            }
        }

        // The new time must be after the old time, so we need to move down in the tree.
        else
        {
            // While the new time is greater than one of its children, move it down.
            uint child1Position = currentPosition<<1;
            uint child2Position = child1Position+1;
            while ((child1Position <= numberReactions && newTime > reactionEvents[reactionQueue[child1Position]].time) ||
                   (child2Position <= numberReactions && newTime > reactionEvents[reactionQueue[child2Position]].time))
            {
                // If only the first child is valid, use it, otherwise use the child with the min time.
                uint minChildPosition = (child2Position > numberReactions)?(child1Position)
                        :((reactionEvents[reactionQueue[child1Position]].time <= reactionEvents[reactionQueue[child2Position]].time)?(child1Position):(child2Position));
                uint minChildReactionIndex = reactionQueue[minChildPosition];

                // Swap the reaction with the child.
                reactionQueue[minChildPosition] = reactionIndex;
                reactionQueue[currentPosition] = minChildReactionIndex;

                // Update the position list.
                reactionPositions[reactionIndex] = minChildPosition;
                reactionPositions[minChildReactionIndex] = currentPosition;

                // Set our new current position and loop again.
                currentPosition = minChildPosition;
                child1Position = currentPosition<<1;
                child2Position = child1Position+1;
            }
        }
    }

protected:
    uint numberReactions;
    ReactionEvent * reactionEvents;
    uint * reactionQueue;
    uint * reactionPositions;
};

}
}

#endif
