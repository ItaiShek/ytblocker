// ytblocker: Add youtube ads to Pi-hole's blacklist automatically
// (c) 2020 Itai Shek

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/ip.h>
#include <netinet/udp.h>
#include <time.h>
#include <stdlib.h>

#define BUFFER_SIZE 65536

void analyzePacket(char *buffer, int packet_size);
void getDNS(char *dns, int dns_size);
void checkIfAd(char *dns);
int ifSubstring(char *dns, char *substr);
void blockAd(char *ad1);
void createAd2(char *ad1, char *ad2);
void timeStamp(char *now);

FILE *logfile;

struct tm *time1;
time_t current_time;
int init_day;



int main()
{
	int packet_size;
	char buffer[BUFFER_SIZE];
	char now[21];

	// initialize time for log
	current_time = time(NULL);
	time1 = localtime(&current_time);
	init_day = time1->tm_mday;

	// Create/Open log file
	logfile = fopen("/var/log/ytdnsblocker.log", "a");
	if(logfile == NULL)
	{
		perror("Could not create log file");
		return 1;
	}

	// Creating the socket
	int sock = socket(AF_INET, SOCK_RAW, IPPROTO_UDP); // Only UDP packets

	if(sock == -1)
	{
		timeStamp(now);
		fprintf(logfile, "%sCould not create socket", now);
		fflush(logfile);
		
		perror("Could not create socket");
		return 1;
	}
	

	while(1)
	{
		
		packet_size = recv(sock, buffer, BUFFER_SIZE, 0);
		if(packet_size == -1)
		{
			timeStamp(now);
			fprintf(logfile, "%sCould not fetch packet", now);
			fflush(logfile);
			perror("Could not fetch packet");
			return 2;
		}

		analyzePacket(buffer, packet_size);
	}

	fflush(logfile);
	close(sock);
	return 0;
}

// The function checks if the packet is a dns address, if it is then the dns string
// is sent to check if it's an ad.
void analyzePacket(char *buffer, int packet_size)
{
	int src_port, dst_port, dns_len;
	char *dns;
	struct udphdr *udp_header;

	// create the udp header
	udp_header = (struct udphdr*)(buffer + ((struct iphdr*)buffer)->ihl*4);

	src_port = ntohs(udp_header->uh_sport);		// source port
	dst_port = ntohs(udp_header->uh_dport);		// destination port

	if(src_port == 53 || dst_port == 53) // DNS protocol is UDP port 53
	{		
		dns = (((char *)udp_header) + ((struct iphdr*)buffer)->ihl*4 + 1);
		dns_len = strlen(dns);
		getDNS(dns, dns_len);

		checkIfAd(dns);
	}
}

// The function gets a dns string and the string length as an input, and fix
// the dns string it to be in the correct form.
void getDNS(char *dns, int dns_size)
{
	int i;
	
	for(i=0; i<dns_size; i++)
	{
		if(dns[i] < 32 || dns[i] > 128)
		{
			dns[i] = '.';
		}
	}

}

// The function check if a given dns is a youtube ad, and if it is then it will get blocked.
void checkIfAd(char *dns)
{
	char *ad1 = "---sn-";		// part of youtube ad dns form
	char *notad = "m4vox";		// not an ad

	if(ifSubstring(dns, ad1) && !ifSubstring(dns, notad))	// if the dns has an ad form block it
	{
		blockAd(dns);
	}
}

// The function checks if the substring "substr" is part of the string "dns".
// If the substring part of the string the function return 1, and if not it return 0.
int ifSubstring(char *dns, char *substr)
{
	int i, j, is_sub = 0;
	int dns_len = strlen(dns);
	int substr_len = strlen(substr);


	for(i=0; i<(dns_len-substr_len); i++)
	{
		for(j=0; j<substr_len; j++)
		{
			if(dns[i+j] != substr[j])	// no match for given character
			{
				is_sub = 0;
				break;
			}
			else						// match for given character
			{
				is_sub = 1;
			}
		}
		if(is_sub == 1)					// substring found
		{
			break;
		}
    }

    return is_sub;
}

// The function gets an ad and block it with "pihole -b" command **********
void blockAd(char *ad1)
{
	int ad1_len = strlen(ad1);
	char ad2[ad1_len-2];

	char now[21];

	char *blockcmd = "pihole -b ";
	char block[strlen(blockcmd) + ad1_len];

	createAd2(ad1, ad2);
	
	timeStamp(now);

	sprintf(block, "%s%s", blockcmd, ad1);
	system(block);
	fprintf(logfile, "%sBlocking ad: %s\n", now, ad1);

	timeStamp(now);
	
	sprintf(block, "%s%s", blockcmd, ad2);
	system(block);
	fprintf(logfile, "%sBlocking ad: %s\n", now, ad2);

	fflush(logfile);
}

// The function will create the sencond ad form from the given ad that the sniffer captured
// E.g.: if ad1 = r2---sn-qxoedn7k.googlevideo.com , than ad2 = r2.sn-qxoedn7k.googlevideo.com
void createAd2(char *ad1, char *ad2)
{
	int i;
	int ad1_len = strlen(ad1);
	int ad2_len = ad1_len-2;
	
	strcpy(ad2, ad1);
	for(i=0; i<ad2_len; i++)
	{
		if(ad1[i] == '-')
		{
			break;
		}
	}
	ad2[i] = '.';
	strncpy(&ad2[i+1], &ad1[i+3], ad2_len);
}

// This function create a time stamp in the form "month day hour:minute:seconds:   "
// Also if the day has changed since last time the program will overwrite the log file
void timeStamp(char *now)
{
	current_time = time(NULL);
	time1 = localtime(&current_time);

	int mon = time1->tm_mon;
	int hour = time1->tm_hour;
	int day = time1->tm_mday;
	int min = time1->tm_min;
	int sec = time1->tm_sec;

	char month[12][4] = { "Jan", "Fab", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"}; 

	if(init_day != day) // If the day has changed overwrite the log file
	{
		fclose(logfile);
		logfile = fopen("/var/log/ytdnsblocker.log", "w");
		if(logfile == NULL)
		{
			perror("Could not create log file");
			exit(1);
		}
		init_day = day;
	}

	sprintf(now, "%s %d %02d:%02d:%02d:   ", month[mon], day, hour, min, sec);	// add the time stamp to now
}
