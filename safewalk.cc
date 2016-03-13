#include <libplayerc++/playerc++.h>
#include <iostream>
#include "communicate.h"
#include "args.h"
#include <string>
#include <time.h>
#include <sstream>
#include <stdlib.h>

#define RAYS 32

//Project 2 for Iain Lee
using namespace PlayerCc;
using namespace std;

//Makes sure that an argument is passed
void check_port(int argc){
	if(argc < 2){
		printf("do ./safewalk port#\n");
		exit(1);
	}
}

//Asks for input from the user
string ask_for_command(){
	string input = "";
	while(input != "start" && input != "exit"){
		cout << "Command? ";
		cin >> input;
	}
	return input;
}

//Empties the listening socket buffer that all robots other than lead will talk to
void empty_listen_buffer(double x, double y, int listen_fd){
	clock_t t = clock();
	float time = 0;
	int nbytes = 0;
	char msg[MAXBUF];
	stringstream ss;

	ss << "6665: (" << x << ", " << y << ")\t";
	string message = ss.str();//Message that will display the ports
	while(time < .05){
		nbytes = listen_to_robot(listen_fd, msg);
		if(nbytes != 0) message += string(msg);//If buffer not empty add to message
		time = (float)(clock() - t)/CLOCKS_PER_SEC;//keeps track of time
	}
	cout << message << endl << endl;
}


/***********************************Motor Functions*************************************/
//Calculates the speed the robot should go
double get_speed(LaserProxy &lp, double l, double r){
	if (l > 100) l = 100;
	if (r > 100) r = 100;
	return (r+l)/1e3;	
}

//Calculates the turning rate of the robot
double get_turn(double l, double r){
	double turn_rate = r - l;
	turn_rate = limit(turn_rate, -40.0, 40.0);
	return dtor(turn_rate);
}

//Calls on the speed and turn functions and sets the speed/turning of the robot
void do_motor_functions(PlayerClient &robot, Position2dProxy &pp, LaserProxy &lp){
	robot.Read();
	double l = 1e5 * lp.GetMinRight() / 500 - 100;
	double r = 1e5 * lp.GetMinLeft() / 500 - 100;
	double newspeed = get_speed(lp, l, r);
	double newturnrate = get_turn(l, r);

	pp.SetSpeed(newspeed, newturnrate);
}

/************************************Robot Actions**************************************/
//Robot control function for leader
void go_lead(PlayerClient &robot, Position2dProxy &pp, LaserProxy &lp, int broadcast_fd){
	float x;
	float time = 0;
	clock_t t = clock();
	string message = "stop";

	pp.SetMotorEnable (true);	
	while(time < .20){//For 20 seconds
		do_motor_functions(robot, pp, lp);
		time = (float)(clock() - t)/CLOCKS_PER_SEC;
	}
	talk_to_all(broadcast_fd, const_cast<char*>(message.c_str()), H);//Tells all to stop
	pp.SetSpeed(0, 0);
	pp.SetMotorEnable(false);
}

//Robot control function for non leader robots
void go(PlayerClient &robot, Position2dProxy &pp, LaserProxy &lp, int listen_fd, 
			int broadcast_fd, int robot_port){
	int nbytes;
	char msg[MAXBUF];
	stringstream ss;

	pp.SetMotorEnable (true);
	while(true){
		nbytes = listen_to_robot(listen_fd, msg);
		if(nbytes != 0 && strcmp(msg, "stop") == 0) break;//stops when it hears stop
		do_motor_functions(robot, pp, lp);
	}
	//tells the leader where it is at and its port number
	ss << robot_port << ": (" << pp.GetXPos() << ", " << pp.GetYPos() << ")\t";
	talk_to_all(broadcast_fd, const_cast<char*>(ss.str().c_str()), H);
	pp.SetSpeed(0, 0);
	pp.SetMotorEnable(false);
}

/*************************************Start Robot***************************************/
//Starts the lead robot
void start_robot_lead(PlayerClient &robot, Position2dProxy &pp, LaserProxy &lp, 
						int broadcast_fd, int listen_fd){
	string input = ask_for_command();//asks user for commands
	{
		if(input == "start"){
			//tells all robots to start
			talk_to_all(broadcast_fd, const_cast<char*>(input.c_str()), H);
			cout << "\nAll robots start safewalking.\n\n";
			go_lead(robot, pp, lp, broadcast_fd);//leader starts
			cout << "20 seconds passed. Current robot locations are: \n\n";
			//empties out listen socket for robot locations
			empty_listen_buffer(pp.GetXPos(), pp.GetYPos(), listen_fd);
			cout << "All robots resume safewalking.\n\n";
			//tells all robots to resume
			talk_to_all(broadcast_fd, const_cast<char*>(input.c_str()), H);
			go_lead(robot, pp, lp, broadcast_fd);//leader starts also
			cout << "20 seconds passed. Mission accomplished.\n\n";
		}
		input = "exit";
		//tells all to exit
		talk_to_all(broadcast_fd, const_cast<char*>(input.c_str()), H);
	}
}

//Starts the non lead robot
void start_robot(PlayerClient &robot, Position2dProxy &pp, LaserProxy &lp, int listen_fd,
					int broadcast_fd, int robot_port){
	char msg[MAXBUF];
	int nbytes = 0;
	while(true){
		nbytes = listen_to_robot(listen_fd, msg);//listens for leader
		if(nbytes != 0 && strcmp(msg, "start") == 0){//if leader says go, go
			go(robot, pp, lp, listen_fd, broadcast_fd, robot_port);
		}
		else if(nbytes != 0 && strcmp(msg, "exit") == 0) return;//leaders says exit, exit
	}

}

/***************************************Main********************************************/
int main(int argc, char **argv){	
	check_port(argc);
	int broadcast_fd;
	int listen_fd;
	int robot_port = atoi(argv[1]);
	bool is_lead = false;
	if(robot_port == 6665){//if leader
		is_lead = true;
		listen_fd = create_listen(PORT_R, H);
		broadcast_fd = create_broadcast(PORT_H, H);
	}
	else if(robot_port >= 6666 && robot_port <= 6671){//if non leader robot
		listen_fd = create_listen(PORT_H, H);
		broadcast_fd = create_broadcast(PORT_R, H);
	}
	else{//bad port or arguments
		printf("no robot exists in that port\n");
		exit(1);
	}

	PlayerClient robot(gHostname, robot_port);
	Position2dProxy pp(&robot, gIndex);
	LaserProxy lp(&robot, gIndex);
	if(is_lead) start_robot_lead(robot, pp, lp, broadcast_fd, listen_fd);
	else start_robot(robot, pp, lp, listen_fd, broadcast_fd, robot_port);
}
