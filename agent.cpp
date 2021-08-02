#include "agent.h"

Agent::Agent()
{
    id = -1;
}

Agent::Agent(int id_v)
{
    id = id_v;
}

Agent::~Agent()
{
    //empty
}

bool status_equal(Status s1, Status s2)
{
    int len = 0;

    if(s1.position == s2.position && s1.task == s2.task && s1.iteration == s2.iteration && s1.tasks_status.size() == s2.tasks_status.size())
    {
        len = s1.tasks_status.size();
        for(int i = 0; i < len; i++)
        {
            if(s1.tasks_status.at(i) != s2.tasks_status.at(i))
            {
                return false;
            }
        }

        return true;
    }

    return false;
}

void Agent::add_state(Status s)
{
    /*Status last;
    if(state_list.size() > 0)
    {
        last = (Status)state_list.back();
        if(!status_equal(last,s))
        {
            state_list.push_back(s);
        }
    }
    else
    {
        state_list.push_back(s);
    }*/
    state_list.push_back(s);
}

vector<Status> Agent::get_states()
{
    return state_list;
}

Action Agent::get_next_action()
{
    Action result;

    if(action_queue.size() > 0)
    {
        result = action_queue.front();
    }

    return result;
}

void Agent::pop_action()
{
    if(action_queue.size() > 0)
    {
        action_queue.pop();
    }
}

queue<Action> Agent::get_actions()
{
    return action_queue;
}

void Agent::add_action(Action a)
{
    /*
    * The first is always moving from the initial location to some milestone
    * so we can simply push it into the stack without validity checking
    */
    if(action_queue.size() > 1)
    {
        Action last = (Action)action_queue.back();

        if(last.type == ACT_TS && a.type != ACT_TF)
        {
            cout << "=============== Agent NO." << this->id <<" ========================" << endl;
            cout << "Last action: task NO." << last.target << " started. " << endl;
            if(a.type == ACT_TS)
            {
                cout << "Wrong type action. Expect TASK FINISH but get task NO." << a.target << " started." << endl;
            }
            if(a.type == ACT_MS)
            {
                cout << "Wrong type action. Expect TASK FINISH but get moving to milestone " << a.target << endl;
            }
            if(a.type == ACT_MF)
            {
                cout << "Wrong type action. Expect TASK FINISH but get arriving at milestone " << a.target << endl;
            }
            cout << "*********************************************************************" << endl;
        }
        if(last.type == ACT_MS && a.type != ACT_MF)
        {
            cout << "=============== Agent NO." << this->id <<" ========================" << endl;
            cout << "Last action: movement to milestone " << last.target << " started. " << endl;
            if(a.type == ACT_TS)
            {
                cout << "Wrong type action. Expect MOVEMENT FINISH but get task NO." << a.target << " started." << endl;
            }
            if(a.type == ACT_TF)
            {
                cout << "Wrong type action. Expect MOVEMENT FINISH but get task NO." << a.target << " finished." << endl;
            }
            if(a.type == ACT_MS)
            {
                cout << "Wrong type action. Expect MOVEMENT FINISH but get moving to milestone " << a.target << endl;
            }
            cout << "*********************************************************************" << endl;
        }

        action_queue.push(a);
    }
    else
    {
        action_queue.push(a);
    }
    
}

string getActionType(Action a)
{
    string result;

    if(a.type == ACT_MS)
    {
        result = "Move Start";
    }
    else if(a.type == ACT_MF)
    {
        result = "Move Finish";
    }
    else if(a.type == ACT_TS)
    {
        result = "Task Start";
    }
    else if(a.type == ACT_TF)
    {
        result = "Task Finish";
    }

    return result;
}