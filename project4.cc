#include <libplayerc++/playerc++.h>
#include <iostream>
#include "communicate.h"
#include "args.h"
#include <string>
#include <queue>

//Project 4 for Iain Lee
using namespace PlayerCc;
using namespace std;

//task manager will broadcast on port R which leader will listen to
//all other robots and task manager will listen on port H where leader will announce to
//global variables
struct Coords{
	double x;
	double y;
};


void broadcast_msg(string msg, int bfd){
	talk_to_all(bfd, const_cast<char*>(msg.c_str()), H);
}

void ask_for_waypoints(queue<string> &commands){
	string input = "";
	while(input != "stop"){
		cout << "Command? ";
		cin >> input;
		commands.push(input);
	}
}

void start_follower(){
}

void go_leader(PlayerClient &robot, Position2dProxy &pp, LaserProxy &lp, int bfd, int lfd){
	
}

void start_leader(int bfd, int lfd){
	PlayerClient robot(gHostname, 6665);
	Position2dProxy pp(&robot, gIndex);
	LaserProxy lp(&robot, gIndex);

	pp.SetMotorEnable(true);
	go_leader(robot, pp, lp, bfd, lfd);
	pp.SetMotorEnable(false);
	//while(true){
		//robot.Read();
		//pp.GoTo((player_pose2d_t){0, 0, 0}, (player_pose2d_t){0.1, 0, 0});
	//}
}

void start_task_manager(int bfd, int lfd){
	char msg[MAXBUF];
	queue<string> commands;
	ask_for_waypoints(commands);
	
	broadcast_msg(commands.front(), bfd);
	commands.pop();
	while(!commands.empty()){
		if(listen_to_robot(lfd, msg) != 0 && strcmp(msg, "Task Complete") == 0){
			broadcast_msg(commands.front(), bfd);
			commands.pop();
		}
	}
}

int main(int argc, char **argv){
	int robot_port = atoi(argv[1]);
	if(robot_port == 1){
		int bfd = create_broadcast(PORT_R, H);
		int lfd = create_listen(PORT_H, H);
		start_task_manager(bfd, lfd);
	}
	else if(robot_port == 6665){
		int bfd = create_broadcast(PORT_H, H);
		int lfd = create_listen(PORT_R, H);
		start_leader(bfd, lfd);
	}
	else if(robot_port >= 6666 && robot_port <= 6668){
		int lfd = create_listen(PORT_H, H);
		start_follower();
	}
	else cout << "no robot exists in port " << robot_port << " try 1 for task manager, 6665 for leader, or 6666-6668 for followers\n";
}
