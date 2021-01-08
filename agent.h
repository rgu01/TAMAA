#ifndef AGENT_H
#define AGENT_H

#include "common.h"

#define ACT_TS 0    //task start
#define ACT_TF 1    //task finish
#define ACT_MS 2    //movement start
#define ACT_MF 3    //movement finish

struct Action
{
    int type; //TS, TF, MS, MF
    int occurance_time;
    int order;
    string target;
};

struct Status
{
    string task;
    string position;
    vector<int> tasks_status;
    int iteration;
};

/*  An agent that moves and performs tasks
*/
class Agent
{
public:
    Agent();
    explicit Agent(int id_v);
    ~Agent();

    int id;

    void add_state(Status s);
    vector<Status> get_states();
    queue<Action> get_actions();
    void add_action(Action a);
    Action get_next_action();
    void pop_action();

private:
    vector<Status> state_list;
    queue<Action> action_queue;
};

string getActionType(Action a);

#endif