#include "common.h"
#include "xmlHandler.h"
#include "xml/tinystr.h"
#include "xml/tinyxml.h"

using namespace std;

bool createXmlFile(const string& fileName, vector<Agent> agents)
{
    int index_agent = 0, index_states = 0, index_tasks = 0, states_size = 0, actions_size[agents.size()];
    int next_task_index = 0, next_task_agent = 0;
    Agent agent_temp;
    Status state_temp;
    Action action_temp, action_next;
    std::vector<Status>::iterator states_it[agents.size()];
    TiXmlDocument *myDocument;
    TiXmlElement *rootElement;

    try
    {
        myDocument = new TiXmlDocument();
        rootElement = new TiXmlElement("Traces");

        states_size = agents[0].get_states().size();
        for(index_agent = 0; index_agent < agents.size(); index_agent++)
        {
            agent_temp = agents[index_agent];
            states_it[index_agent] = agent_temp.get_states().begin();
            actions_size[index_agent] = agent_temp.get_states().size();
        }

        //states nodes
        for(index_states = 0; index_states < states_size; index_states++)
        {        
            TiXmlElement *stateElement = new TiXmlElement("State");
            for(index_agent = 0; index_agent < agents.size(); index_agent++)
            {
                agent_temp = agents[index_agent];
                state_temp = agent_temp.get_states()[index_states];
                TiXmlElement *agentElement = new TiXmlElement("Agent");
                TiXmlElement *milestoneElement = new TiXmlElement("Milestone");
                TiXmlText *milestoneContent = new TiXmlText(state_temp.position.c_str());
                milestoneElement->LinkEndChild(milestoneContent);
                TiXmlElement *taskElement = new TiXmlElement("Task");
                TiXmlText *taskContent = new TiXmlText(state_temp.task.c_str());
                taskElement->LinkEndChild(taskContent);
                TiXmlElement *finishElement = new TiXmlElement("Finish");
                for(index_tasks = 0; index_tasks < state_temp.tasks_status.size(); index_tasks++)
                {
                    if(state_temp.tasks_status[index_tasks] == 1)
                    {
                        TiXmlElement *finTaskElement = new TiXmlElement("Task");
                        TiXmlText *finTaskContent = new TiXmlText(("T" + to_string(index_tasks)).c_str());
                        finTaskElement->LinkEndChild(finTaskContent);
                        finishElement->LinkEndChild(finTaskElement);
                    }
                }
                TiXmlElement *iterationElement = new TiXmlElement("Iteration");
                TiXmlText *iterationContent = new TiXmlText(to_string(state_temp.iteration).c_str());
                iterationElement->LinkEndChild(iterationContent);
                
                agentElement->SetAttribute("id", agent_temp.id);
                agentElement->LinkEndChild(milestoneElement);
                agentElement->LinkEndChild(taskElement);
                agentElement->LinkEndChild(finishElement);
                agentElement->LinkEndChild(iterationElement);

                stateElement->LinkEndChild(agentElement);
            }

            rootElement->LinkEndChild(stateElement);
            //action
            for(index_agent = 0; index_agent < agents.size(); index_agent++)
            {
                agent_temp = agents[index_agent];
                action_temp = agent_temp.get_next_action();
                if(next_task_index == action_temp.order)
                {
                    action_next = action_temp;
                    next_task_agent = index_agent;
                    next_task_index++;
                    agents[index_agent].pop_action();
                    break;
                }
            }
            //action node
            TiXmlElement *actionElement = new TiXmlElement("Action");
            TiXmlElement *agentElement = new TiXmlElement("Agent");
            agentElement->SetAttribute("id", next_task_agent);

            TiXmlElement *typeElement = new TiXmlElement("Type");
            TiXmlText *typeContent = new TiXmlText(getActionType(action_next).c_str());
            typeElement->LinkEndChild(typeContent);

            TiXmlElement *targetElement = new TiXmlElement("Target");
            TiXmlText *targetContent = new TiXmlText(action_next.target.c_str());
            targetElement->LinkEndChild(targetContent);

            TiXmlElement *timeElement = new TiXmlElement("Time");
            TiXmlText *timeContent = new TiXmlText(to_string(action_next.occurance_time).c_str());
            timeElement->LinkEndChild(timeContent);

            agentElement->LinkEndChild(typeElement);
            agentElement->LinkEndChild(targetElement);
            agentElement->LinkEndChild(timeElement);

            actionElement->LinkEndChild(agentElement);

            rootElement->LinkEndChild(actionElement);
        }

        myDocument->LinkEndChild(rootElement);
        myDocument->SaveFile(fileName.c_str());
    }
    catch (string& e)
    {
        return false;
    }
    return true;
}

bool readXmlFile(string& szFileName)
{//读取Xml文件，并遍历
    try
    {
        //CString appPath = GetAppPath();
        string seperator = "\\";
        //string fullPath = appPath.GetBuffer(0) +seperator+szFileName;
        string fullPath = "";
        //创建一个XML的文档对象。
        TiXmlDocument *myDocument = new TiXmlDocument(fullPath.c_str());
        myDocument->LoadFile();
        //获得根元素，即Persons。
        TiXmlElement *RootElement = myDocument->RootElement();
        //输出根元素名称，即输出Persons。
        cout << RootElement->Value() << endl;
        //获得第一个Person节点。
        TiXmlElement *FirstPerson = RootElement->FirstChildElement();
        //获得第一个Person的name节点和age节点和ID属性。
        TiXmlElement *NameElement = FirstPerson->FirstChildElement();
        TiXmlElement *AgeElement = NameElement->NextSiblingElement();
        TiXmlAttribute *IDAttribute = FirstPerson->FirstAttribute();
        //输出第一个Person的name内容，即周星星；age内容，即；ID属性，即。
        cout << NameElement->FirstChild()->Value() << endl;
        cout << AgeElement->FirstChild()->Value() << endl;
        cout << IDAttribute->Value()<< endl;
    }
    catch (string& e)
    {
        return false;
    }
    return true;
}