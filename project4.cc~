#include <libplayerc++/playerc++.h>
#include <iostream>
#include "communicate.h"
#include "args.h"
#include <string>
#include <queue>
#include <sstream>
#include <stdlib.h>

//Project 4 for Iain Lee
using namespace PlayerCc;
using namespace std;

//Holds x, y, and form information as one object
struct Coords{
	double x;
	double y;
	double form;
};

//Globals
int robot_port;

//Simplified broadcast method
void broadcast_msg(string msg, int bfd){
	talk_to_all(bfd, const_cast<char*>(msg.c_str()), H);
}

/**********************************Follower Methods*************************************/
//Receives a string and pushes the information into the Coords struct
void grab_leader_position(string msg, Coords &coord){
	istringstream splitter(msg);
	istream_iterator<string> beg(splitter), end;
	vector<string> tokens(beg, end);
	coord.form = atof(tokens[0].c_str());
	coord.x = atof(tokens[1].c_str());
	coord.y = atof(tokens[2].c_str());	
}

//Follower Movement Function
void go_followers(PlayerClient** &robots, Position2dProxy** &ppp, int bfd, int lfd, 
					queue<string> &commands){
	char msg[MAXBUF];
	Coords coord;
	bool started = false;//Checks to see if things have started
	bool change_pauser = true;//Pauses followers when changing from line to diamond
	broadcast_msg(commands.front(), bfd);
	commands.pop();
	while(true){
		if(listen_to_robot(lfd, msg) != 0){//Listener for Leader
			if(strcmp(msg, "Task Complete") == 0){//Gives to task manager
				broadcast_msg(commands.front(), bfd);//Gives next command to leader
				commands.pop();
			}
			else{//Gives position of leader to followers
				grab_leader_position(string(msg), coord);
				started = true;
				string message = "received";
				broadcast_msg(message, bfd);
			}
		}
		if(started){
			for(int i = 0; i < 3; i++) robots[i] -> Read();
			if(coord.form == 0){//Line Formation
				ppp[0] -> GoTo(coord.x, coord.y + 0.967, 0);
				ppp[1] -> GoTo(coord.x, coord.y - 1.035, 0);
				ppp[2] -> GoTo(coord.x, coord.y - 2.035, 0);
				change_pauser = true;
			}
			else{
				if(change_pauser){//Slow down for diamond
					ppp[2] -> SetSpeed(0, 0);
					ppp[0] -> SetSpeed(.1, 0);
					ppp[1] -> SetSpeed(.1, 0);
					sleep(1);
					change_pauser = false;
				} 
				else{//Diamond formation
					ppp[0] -> GoTo(coord.x - 1, coord.y + 0.967, 0);
					ppp[1] -> GoTo(coord.x - 1, coord.y - 1.035, 0);
					ppp[2] -> GoTo(coord.x - 2, coord.y, 0);
				}
			}
		}
	}
}

//Creates the robot through pointer arrays
void create_robot(int port, PlayerClient** &robots, Position2dProxy** &ppp, 
					int i){
	robots[i] = new PlayerClient(gHostname, port);
	ppp[i] = new Position2dProxy(robots[i], gIndex);
}

//Creates the pointers to pointers to hold all the robots
void start_followers(int bfd, int lfd, queue<string> &commands){
	PlayerClient** robots = new PlayerClient*[3];//Pointer to array of robots
	Position2dProxy** ppp = new Position2dProxy*[3];//Pointer to array of proxies

	for(int i = 0; i < 3; i++){
		create_robot(6666 + i, robots, ppp, i);
		ppp[i] -> SetMotorEnable(true);
	}
	go_followers(robots, ppp, bfd, lfd, commands);
	for(int i = 0; i < 3; i++){
		ppp[i] -> SetSpeed(0, 0);
		ppp[i] -> SetMotorEnable(false);
	}
}

/********************************Task Manager Methods***********************************/
//Asks users for the commands to give
void ask_for_waypoints(queue<string> &commands){
	string input = "";
	while(input != "stop"){
		cout << "Command? ";
		getline(cin, input);
		commands.push(input);
	}
}


//Starts the Task manager
void start_task_manager(int bfd, int lfd){
	char msg[MAXBUF];
	queue<string> commands;
	ask_for_waypoints(commands);
	
	start_followers(bfd, lfd, commands);
}

/***********************************Leader Methods**************************************/
//Converts the message from task manager into a readable array
void grab_command(string msg, double command[]){
	istringstream splitter(msg);
	istream_iterator<string> beg(splitter), end;
	string line = "line";
	vector<string> tokens(beg, end);
	if(tokens[0].compare(line) == 0) command[0] = 0;
	else command[0] = 1;
	command[1] = atof(tokens[1].c_str());
	command[2] = atof(tokens[2].c_str());
}

//Leader movement function
void go_leader(PlayerClient &robot, Position2dProxy &pp, int bfd, int lfd){
	char msg[MAXBUF];
	double comm[3];
	bool started = false;
	string complete = "Task Complete";
	bool send = true;//Did leader broadcast to followers
	bool is_line = true;//Is it line formation
	while(true){
		stringstream ss;
		if(listen_to_robot(lfd, msg) != 0){//Listens to followers and task manager
			if(strcmp(msg, "stop") == 0) return;//Listens to task manager
			else if(strcmp(msg, "received") == 0) send = true;//If follower has received
			//can now send next position
			else{
				grab_command(string(msg), comm);
				started = true;
			}
		}
		if(started){
			robot.Read();
			double dx = comm[1] - pp.GetXPos();
			double dy = comm[2] - pp.GetYPos();
			double distance = sqrt(dx * dx + dy * dy);
			pp.GoTo(comm[1], comm[2], 0);	
			if(comm[0] == 1) is_line = false;
			if(distance < .5){//If close to waypoint
				if(pp.GetXSpeed() == 0){//Sends task complete
					broadcast_msg(complete, bfd);
					started = false;
				}
				if(send){//Sends to followers the current position of leader
					ss << comm[0] << " " << pp.GetXPos() << " " << pp.GetYPos();
					broadcast_msg(ss.str(), bfd);	
					send = false;
					if(!is_line && comm[0] == 0){
						pp.SetSpeed(0, 0);
						is_line = true;
						sleep(2);
					} 
				}
			}
			else{
				if(send){
					if(!is_line && comm[0] == 0){//If changing to line from diamond stop
						ss << comm[0] << " " << pp.GetXPos() << " " << pp.GetYPos();
						broadcast_msg(ss.str(), bfd);
						pp.SetSpeed(0, 0);
						is_line = true;
						sleep(12);
					} 
					else{//gives followers a futher x position so goto doens't slow down
						ss << comm[0] << " " << pp.GetXPos() + .5 << " " << pp.GetYPos();
						broadcast_msg(ss.str(), bfd);
					}
					send = false;
				} 
			}
		}
	}
}

//Creates leader robot
void start_leader(int bfd, int lfd){
	PlayerClient robot(gHostname, 6665);
	Position2dProxy pp(&robot, gIndex);

	pp.SetMotorEnable(true);
	go_leader(robot, pp, bfd, lfd);
	pp.SetSpeed(0, 0);
	pp.SetMotorEnable(false);
}

/****************************************Main*******************************************/
int main(int argc, char **argv){
	robot_port = atoi(argv[1]);
	if(robot_port == 1){
		int bfd = create_broadcast(PORT_R, H);
		int lfd = create_listen(PORT_H, H);
		start_task_manager(bfd, lfd);
	}
	else if(robot_port == 2){
		int bfd = create_broadcast(PORT_H, H);
		int lfd = create_listen(PORT_R, H);
		start_leader(bfd, lfd);
	}
	else cout << "no robot exists in port " << robot_port 
			<< " try 1 for task manager and followers or 2 for leader\n";
}
