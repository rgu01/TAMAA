// -*- mode: C++; c-file-style: "stroustrup"; c-basic-offset: 4; indent-tabs-mode: nil; -*-

/* tracer - Utility for printing UPPAAL XTR trace files.
   Copyright (C) 2006 Uppsala University and Aalborg University.
   Copyright (C) 2017 Aalborg University.

   This library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public License
   as published by the Free Software Foundation; either version 2.1 of
   the License, or (at your option) any later version.

   This library is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with this library; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA
*/

#include "tracer.h"

/* This utility takes an UPPAAL model in the UPPAAL intermediate
 * format and a UPPAAL XTR trace file and prints trace to stdout in a
 * human readable format.
 *
 * The utility basically contains two parsers: One for the
 * intermediate format and one for the XTR format. You may want to use
 * them a starting point for writing analysis tools.
 *
 * Notice that the intermediate format uses a global numbering of
 * clocks, variables, locations, etc. This is in contrast to the XTR
 * format, which makes a clear distinction between e.g. clocks and
 * variables and uses process local number of locations and
 * edges. Care must be taken to convert between these two numbering
 * schemes.
 */


/* Representation of a memory cell.
 */
struct cell_t
{
    enum type_t: int { CONST, CLOCK, VAR, META, SYS_META, COST, LOCATION, FIXED };
    enum flags_t: int { NONE, COMMITTED, URGENT };
    /** The type of the cell. */
    type_t type;

    /** Name of cell. Not all types have names. */
    string name;

    union
    {
        int value;
        struct
        {
            int nr;
        } clock;
        struct
        {
            int min;
            int max;
            int init;
            int nr;
        } var;
        struct
        {
            int min;
            int max;
            int init;
            int nr;
        } meta;
        struct
        {
            int min;
            int max;
        } sys_meta;
        struct
        {
            flags_t flags;
            int process;
            int invariant;
        } location;
        struct
        {
            int min;
            int max;
        } fixed;
    };
};

/* Representation of a process.
 */
struct process_t
{
    int initial;
    string name;
    vector<int> locations;
    vector<int> edges;
};

/* Representation of an edge.
 */
struct edge_t
{
    int process;
    int source;
    int target;
    int guard;
    int sync;
    int update;
};

/* The UPPAAL model in intermediate format.
 */
vector<cell_t> layout;
vector<int> instructions;
vector<process_t> processes;
vector<edge_t> edges;
map<int,string> expressions;

/* For convenience we keep the size of the system here.
 */
size_t processCount = 0;
size_t variableCount = 0;
size_t clockCount = 0;

/* These are mappings from variable and clock indicies to
 * the names of these variables and clocks.
 */
vector<string> clocks;
vector<string> variables;

/* Thrown by parser upon parse errors.
 */
class invalid_format : public std::runtime_error
{
public:
    explicit invalid_format(const string&  arg) : runtime_error(arg) {}
};

/* Reads one line from file. Skips comments.
 */
bool read(istream& file, string& str)
{
    do
    {
        if (!getline(file, str))
        {
            return false;
        }
    } while (!str.empty() && str[0] == '#');
    return true;
}

/* Reads one line and asserts that it contains a (terminating) dot
 */
istream& readdot(istream& is)
{
    string str;
    getline(is, str);
    if (str.empty())
    {
        getline(is, str);
    }
    if (str != ".")
    {
        cerr << "Expecting a line with '.' but got '" << str << "'" << endl;
        assert(false);
        exit(EXIT_FAILURE);
    }
    return is;
}

inline
istream& skipspaces(istream& is)
{
    while (is.peek() == ' ')
    {
        is.get();
    }
    return is;
}

/* Parser for intermediate format.
 */
void loadIF(istream& file)
{
    string str;
    string section;
    char name[32];
    int index;
    while (getline(file, section))
    {
        if (section == "layout")
        {
            cell_t cell;
            while (read(file, str) && !str.empty() && !isspace(str[0]))
            {
                char s[5];
                auto cstr = str.c_str();

                if (sscanf(cstr, "%d:clock:%d:%31s", &index,
                           &cell.clock.nr, name) == 3)
                {
                    cell.type = cell_t::CLOCK;
                    cell.name = name;
                    clocks.emplace_back(name);
                    clockCount++;
                }
                else if (sscanf(cstr, "%d:const:%d", &index,
                                &cell.value) == 2)
                {
                    cell.type = cell_t::CONST;
                }
                else if (sscanf(cstr, "%d:var:%d:%d:%d:%d:%31s", &index,
                                &cell.var.min, &cell.var.max, &cell.var.init,
                                &cell.var.nr, name) == 6)
                {
                    cell.type = cell_t::VAR;
                    cell.name = name;
                    variables.emplace_back(name);
                    variableCount++;
                }
                else if (sscanf(cstr, "%d:meta:%d:%d:%d:%d:%31s", &index,
                                &cell.meta.min, &cell.meta.max, &cell.meta.init,
                                &cell.meta.nr, name) == 6)
                {
                    cell.type = cell_t::META;
                    cell.name = name;
                    variables.emplace_back(name);
                    variableCount++;
                }
                else if (sscanf(cstr, "%d:sys_meta:%d:%d:%31s", &index,
                                &cell.sys_meta.min, &cell.sys_meta.max, name) == 4)
                {
                    cell.type = cell_t::SYS_META;
                    cell.name = name;
                }
                else if (sscanf(cstr, "%d:location::%31s", &index, name) == 2)
                {
                    cell.type = cell_t::LOCATION;
                    cell.location.flags = cell_t::NONE;
                    cell.name = name;
                }
                else if (sscanf(cstr, "%d:location:committed:%31s", &index, name) == 2)
                {
                    cell.type = cell_t::LOCATION;
                    cell.location.flags = cell_t::COMMITTED;
                    cell.name = name;
                }
                else if (sscanf(cstr, "%d:location:urgent:%31s", &index, name) == 2)
                {
                    cell.type = cell_t::LOCATION;
                    cell.location.flags = cell_t::URGENT;
                    cell.name = name;
                }
                else if (sscanf(cstr, "%d:static:%d:%d:%31s", &index,
                                &cell.fixed.min, &cell.fixed.max,
                                name) == 4)
                {
                    cell.type = cell_t::FIXED;
                    cell.name = name;
                }
                else if (sscanf(cstr, "%d:%5s", &index, s) == 2
                         && strcmp(s, "cost") == 0)
                {
                    cell.type = cell_t::COST;
                }
                else
                {
                    throw invalid_format(str);
                }

                layout.push_back(cell);
            }
#if defined(ENABLE_CORA) || defined(ENABLE_PRICED)
            cell.type = cell_t::VAR;
            cell.var.min = std::numeric_limits<int32_t>::min();
            cell.var.max = std::numeric_limits<int32_t>::max();
            cell.var.init = 0;

            cell.name = "infimum_cost";
            cell.var.nr = variableCount++;
            variables.push_back(cell.name);
            layout.push_back(cell);

            cell.name = "offset_cost";
            cell.var.nr = variableCount++;
            variables.push_back(cell.name);
            layout.push_back(cell);

            for (size_t i=1; i<clocks.size(); ++i) {
                cell.name = "#rate[";
                cell.name.append(clocks[i]);
                cell.name.append("]");
                cell.var.nr = variableCount++;
                variables.push_back(cell.name);
                layout.push_back(cell);
            }
#endif
        }
        else if (section == "instructions")
        {
            while (read(file, str) && !str.empty() && (!isspace(str[0]) || str[0]=='\t'))
            {
                int address;
                int values[4];
                if (str[0]=='\t')
                {   // skip pretty-printed instruction text
                    continue;
                }
                int cnt = sscanf(str.c_str(), "%d:%d%d%d%d", &address,
                                 &values[0], &values[1], &values[2], &values[3]);
                if (cnt < 2)
                {
                    throw invalid_format("In instruction section");
                }

                for (int i = 0; i < cnt-1; ++i)
                {
                    instructions.push_back(values[i]);
                }
            }
        }
        else if (section == "processes")
        {
            while (read(file, str) && !str.empty() && !isspace(str[0]))
            {
                process_t process;
                if (sscanf(str.c_str(), "%d:%d:%31s",
                           &index, &process.initial, name) != 3)
                {
                    throw invalid_format("In process section");
                }
                process.name = name;
                processes.push_back(process);
                processCount++;
            }
        }
        else if (section == "locations")
        {
            while (read(file, str) && !str.empty() && !isspace(str[0]))
            {
                int index;
                int process;
                int invariant;

                if (sscanf(str.c_str(), "%d:%d:%d", &index, &process, &invariant) != 3)
                {
                    throw invalid_format("In location section");
                }

                layout[index].location.process = process;
                layout[index].location.invariant = invariant;
                processes[process].locations.push_back(index);
            }
        }
        else if (section == "edges")
        {
            while (read(file, str) && !str.empty() && !isspace(str[0]))
            {
                edge_t edge;

                if (sscanf(str.c_str(), "%d:%d:%d:%d:%d:%d", &edge.process,
                           &edge.source, &edge.target,
                           &edge.guard, &edge.sync, &edge.update) != 6)
                {
                    throw invalid_format("In edge section");
                }

                processes[edge.process].edges.push_back(edges.size());
                edges.push_back(edge);
            }
        }
        else if (section == "expressions")
        {
            while (read(file, str) && !str.empty() && !isspace(str[0]))
            {
                if (sscanf(str.c_str(), "%d", &index) != 1)
                {
                    throw invalid_format("In expression section");
                }

                /* Find expression string (after the third colon). */
                auto *s = str.c_str();
                int cnt = 3;
                while (cnt && *s)
                {
                    cnt -= (*s == ':');
                    s++;
                }
                if (cnt)
                {
                    throw invalid_format("In expression section");
                }

                /* Trim white space. */
                while (*s && isspace(*s))
                {
                    s++;
                }
                auto *t = s + strlen(s) - 1;
                while (t >= s && isspace(*t))
                {
                    t--;
                }

                expressions[index] = string(s, t+1);
            }
        }
        else
        {
            throw invalid_format("Unknown section");
        }
    }
};

/* A bound for a clock constraint. A bound consists of a value and a
 * bit indicating whether the bound is strict or not.
 */
struct bound_t
{
    int value   : 31; // The value of the bound
    bool strict : 1;  // True if the bound is strict
};

/* The bound (infinity, <).
 */
static bound_t infinity = { std::numeric_limits<int32_t>::max() >> 1, true };

/* The bound (0, <=).
 */
static bound_t zero = { 0, false };

/* A symbolic state. A symbolic state consists of a location vector, a
 * variable vector and a zone describing the possible values of the
 * clocks in a symbolic manner.
 */
class State
{
public:
    State();
    explicit State(istream& file);
    State(const State& s) = delete;
    State(State&& s) = delete;
    ~State();

    int &getLocation(int i)              { return locations[i]; }
    int &getVariable(int i)              { return integers[i]; }
    bound_t &getConstraint(int i, int j) { return dbm[i * clockCount + j]; }

    int getLocation(int i) const              { return locations[i]; }
    int getVariable(int i) const              { return integers[i]; }
    bound_t getConstraint(int i, int j) const { return dbm[i * clockCount + j]; }
    int id;
private:
    vector<int> locations;
    vector<int> integers;
    bound_t *dbm;
    void allocate();
};

State::~State()
{
    delete[] dbm;
}

State::State()
{
    /* Allocate. */
    locations.resize(processCount);
    integers.resize(variableCount);
    dbm = new bound_t[clockCount * clockCount];

    /* Fill with default values. */
    std::fill(dbm, dbm + clockCount * clockCount, infinity);

    /* Set diagonal and lower bounds to zero. */
    for (size_t i = 0; i < clockCount; i++)
    {
        getConstraint(0, i) = zero;
        getConstraint(i, i) = zero;
    }
}

State::State(istream& file): State()
{
    /* Read locations.  */
    for (auto& l: locations)
    {
        file >> l;
    }
    file >> skipspaces;
    file >> readdot;

    /* Read DBM. */
    int i, j, bnd;
    while (file >> i >> j >> bnd)
    {
        file >> readdot;
        getConstraint(i, j).value = bnd >> 1;
        getConstraint(i, j).strict = bnd & 1;
    }
    file.clear();
    file >> skipspaces;
    file >> readdot;

    /* Read integers. */
    for (auto& v: integers)
    {
        file >> v;
    }
    file >> skipspaces;
    file >> readdot;
}

struct Edge
{
    int process;
    int edge;
    vector<int> select;
};

/* A transition consists of one or more edges. Edges are indexes from
 * 0 in the order they appear in the input file.
 */
struct Transition
{
    vector<Edge> edges;
    explicit Transition(istream& file);
};

Transition::Transition(istream& file)
{
    int process, edge, select;
    while (file >> process >> edge)
    {
        Edge e{process, edge};
        file >> skipspaces;
        while (file.peek()!='\n' && file.peek()!=';')
        {
            if (file >> select)
            {
                e.select.push_back(select);
            }
            else
            {
                cerr << "Transition format error" << endl;
                exit(EXIT_FAILURE);
            }
            file >> skipspaces;
        }
        if (file.get()=='\n') // old format without ';'
        {   // old format indexes edges from 1, hence convert to 0-base
            e.edge--;
        }
        edges.push_back(e);
    }
    file.clear();
    file >> readdot;
}

/* Output operator for a symbolic state. Prints the location vector,
 * the variables and the zone of the symbolic state.
 */
ostream &operator << (ostream &o, const State &state)
{
    /* Print location vector. */
    for (size_t p = 0; p < processCount; p++)
    {
        int idx = processes[p].locations[state.getLocation(p)];
        cout << processes[p].name << '.' << layout[idx].name << " ";
    }

    /* Print variables. */
    for (size_t v = 0; v < variableCount; v++)
    {
        cout << variables[v] << "=" << state.getVariable(v) << ' ';
    }

    /* Print clocks. */
    for (size_t i = 0; i < clockCount; i++)
    {
        for (size_t j = 0; j < clockCount; j++)
        {
            if (i != j)
            {
                bound_t bnd = state.getConstraint(i, j);

                if (bnd.value != infinity.value)
                {
                    cout << clocks[i] << "-" << clocks[j]
                         << (bnd.strict ? "<" : "<=") << bnd.value << " ";
                }
            }
        }
    }

    return o;
}

/* Output operator for a transition. Prints all edges in the
 * transition including the source, destination, guard,
 * synchronisation and assignment.
 */
ostream &operator << (ostream &o, const Transition &t)
{
    for (auto& edge: t.edges)
    {
        int eid = processes[edge.process].edges[edge.edge];
        int src = edges[eid].source;
        int dst = edges[eid].target;
        int guard = edges[eid].guard;
        int sync = edges[eid].sync;
        int update = edges[eid].update;
        cout << processes[edge.process].name << '.' << layout[src].name
             << " -> "
             << processes[edge.process].name << '.' << layout[dst].name;
        if (!edge.select.empty()) {
            auto s=edge.select.begin(), se=edge.select.end();
            cout << " [" << *s;
            while (++s != se) cout << "," << *s;
            cout << "]";
        }
        cout << " {"
             << expressions[guard] << "; " << expressions[sync] << "; " << expressions[update]
             << ";} ";
    }

    return o;
}

/* Read and print a trace file.
 */
int loadTrace(istream& file)
{
    int satisfied = 0;

    /* Read and print trace. */
    cout << "State: " << State(file) << endl;
    for (;;)
    {
        /* Skip white space. */
        file >> skipspaces;

        /* A dot terminates the trace. */
        if (file.peek() == '.')
        {
            file.get();
            break;
        }

        /* Read a state and a transition. */
        State state(file);
        Transition transition(file);

        /* Print transition and state. */
        cout << "\nTransition: " << transition << endl
             << "\nState: " << state << endl;
    }

    return satisfied;
}

static string const_str_move = "movement";
static string const_str_task = "taskExe";
static string const_str_initial_position = "initial";
static string const_str_iteration = "iteration";
//static string const_str_ts = "ts";
static string const_str_tf = "tf";
static string const_str_time = "t >= ";

void createState(State& state, vector<Agent>& agents, vector<Status>& cs)
{
    string name = "";
    int index, agent_id_i, task_i, id_x, index_actions = 0;
    int eid, src, dst, guard, sync, update;
    int const_len_move = const_str_move.size();
    int const_len_task = const_str_task.size();
    int const_len_iteration = const_str_iteration.size();
    int const_len_tf = const_str_tf.size();
    int const_len_time = const_str_time.size();
    //Status cs/[agents.size()];
    //State: location
    for (size_t p = 0; p < processCount; p++)
    {
        id_x = processes[p].locations[state.getLocation(p)];
        name = processes[p].name;
        index = name.find(const_str_move);
        if(index != string::npos)
        {
            // movement TA
            agent_id_i = stoi(name.substr(const_len_move, const_len_move));
            if(agent_id_i < agents.size())
            {
                cs[agent_id_i].position = layout[id_x].name;
            }
        }
        else
        {
            index = name.find(const_str_task);
            if(index != string::npos)
            {
                //task TA
                agent_id_i = stoi(name.substr(const_len_task, const_len_task));
                if(agent_id_i < agents.size())
                {
                    cs[agent_id_i].task = layout[id_x].name;
                }
            }
        }
    }
    //State: variables
    for (size_t v = 0; v < variableCount; v++)
    {
        index = variables[v].find(const_str_iteration);
        if(index != string::npos)
        {
            //iteration
            agent_id_i = stoi(variables[v].substr(const_len_iteration + 1, const_len_iteration + 1));
            if(agent_id_i < agents.size())
            {
                cs[agent_id_i].iteration = state.getVariable(v);
            }
        }
        index = variables[v].find(const_str_tf);
        if(index != string::npos)
        {
            //finished tasks
            agent_id_i = stoi(variables[v].substr(const_len_tf + 1, const_len_tf +1 ));
            task_i = stoi(variables[v].substr(const_len_tf + 4, const_len_task + const_len_tf + 4));
            if(agent_id_i < agents.size())
            {
                if(task_i < cs[agent_id_i].tasks_status.size())
                {
                    cs[agent_id_i].tasks_status[task_i] = state.getVariable(v);
                }
                else
                {
                    cs[agent_id_i].tasks_status.push_back(state.getVariable(v));
                } 
            }
        }
    }
}

void clear()
{
    layout.clear();
    instructions.clear();
    processes.clear();
    edges.clear();
    expressions.clear();

    processCount = 0;
    variableCount = 0;
    clockCount = 0;

    clocks.clear();
    variables.clear();
}

int parseTrace(istream& file, vector<Agent>& agents)
{
    string name = "";
    int index, id_i, task_i, id_x, index_actions = 0;
    int eid, src, dst, guard, sync, update;
    int const_len_move = const_str_move.size();
    int const_len_task = const_str_task.size();
    int const_len_iteration = const_str_iteration.size();
    int const_len_tf = const_str_tf.size();
    int const_len_time = const_str_time.size();
    vector<Status> cs;
    int satisfied = 0;
    
    if(file.peek() == EOF)
    {
        satisfied = 0;
        printf("NOT satisfied. No XML generated!\n");
    }
    else
    {
        for (size_t p = 0; p < processCount; p++)
        {
            name = processes[p].name;
            index = name.find(const_str_move);
            if(index != string::npos)
            {
                Agent agent(0);
                id_i = stoi(processes[p].name.substr(const_len_move, const_len_move));
                agent.id = id_i;
                agents.push_back(agent);
            }
        }

        for(int i = 0; i < agents.size(); i++)
        {
            cs.push_back(Status());
        }
        //cout << "State: " << State(file) << endl;
        State state(file); 
        createState(state, agents, cs);
        //add the states into agents
        for(int i = 0; i < agents.size(); i++)
        {
            agents[i].add_state(cs[i]);
        }       

        for (;;)
        {
            /* Skip white space. */
            file >> skipspaces;

            /* A dot terminates the trace. */
            if (file.peek() == '.')
            {
                file.get();
                break;
            }
            /* Read a state and a transition. */
            State state(file);
            Transition transition(file);

            /* Print transition and state. */
            //cout << "\nTransition: " << transition << endl
            //     << "\nState: " << state << endl;
            createState(state, agents, cs);

            //Transition
            Action action;
            for (auto& edge: transition.edges)
            {
                id_i = -1;
                eid = processes[edge.process].edges[edge.edge];
                src = edges[eid].source;
                dst = edges[eid].target;
                guard = edges[eid].guard;
                sync = edges[eid].sync;
                update = edges[eid].update;
                name = processes[edge.process].name;

                //int test_i = expressions[sync].compare("0");
                //int test_i_layout = layout[src].name.compare(const_str_initial_position);
                //string test_s = expressions[sync];
                //string test_s_layout = layout[src].name;

                //eid = layout[src].name.compare("const_str_initial_position");

                if(expressions[sync] != "0" || layout[src].name == const_str_initial_position)
                {
                    //start to move
                    index = name.find(const_str_move);
                    if(index != string::npos)
                    {
                        id_i = stoi(name.substr(const_len_move, const_len_move));
                        action.type = ACT_MS;
                        action.target = layout[dst].name;
                        action.occurance_time = 0;
                        action.order = index_actions;
                        index_actions++;
                    }
                }
                else
                {
                    index = name.find(const_str_move);
                    if(index != string::npos)
                    {
                        //finish to move
                        id_i = stoi(name.substr(const_len_move, const_len_move));
                        action.type = ACT_MF;
                        action.target = layout[dst].name;
                        action.occurance_time = stoi(expressions[guard].substr(const_len_time, const_len_time));
                        action.order = index_actions;
                        index_actions++;
                    }
                    else
                    {
                        index = name.find(const_str_task);
                        if(index != string::npos)
                        {
                            if(layout[src].name == "T0")
                            {
                                //start a task
                                action.type = ACT_TS;
                                action.target = layout[dst].name;
                                action.occurance_time = 0;
                                action.order = index_actions;
                                index_actions++;
                            }
                            else
                            {
                                //finish a task
                                action.type = ACT_TF;
                                action.target = "T0";
                                action.occurance_time = stoi(expressions[guard].substr(const_len_time, const_len_time));
                                action.order = index_actions;
                                index_actions++;
                            } 
                        }
                        id_i = stoi(name.substr(const_len_task, const_len_task));
                    }
                }
            }
            //find the agent
            for(int i = 0; i < agents.size(); i++)
            {
                agents[i].add_state(cs[i]);
            }
            if(id_i<0)
            {
                assert(id_i<0);
            }
            assert(id_i>=0);
            agents[id_i].add_action(action);
        }

        printf("Satisfied. A XML generated!\n");
        satisfied = 1;
    }

    return satisfied;
}

int parse(int argc, char *argv[])
{
    try
    {
        if (argc < 3)
        {
            printf("Synopsis: %s <if> <trace>\n", argv[0]);
            exit(1);
        }

        /* Load model in intermediate format.
         */
        if (strcmp(argv[1], "-") == 0)
        {
            loadIF(std::cin);
        }
        else
        {
            ifstream file(argv[1]);
            if (!file)
            {
                perror(argv[1]);
                exit(EXIT_FAILURE);
            }
            loadIF(file);
            file.close();
        }

        /* Load trace.
         */
        ifstream file(argv[2]);
        if (!file)
        {
            perror(argv[2]);
            exit(EXIT_FAILURE);
        }
        loadTrace(file);
        file.close();
    }
    catch (std::exception &e)
    {
        cerr << "Cought exception: " << e.what() << endl;
    }

    return 0;
}
