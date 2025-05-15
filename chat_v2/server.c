#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/epoll.h>
#include <pthread.h>

#define PORTNUM 9000
#define ROOM0   0
#define MAX_CHATROOMS  20 
#define MAX_USERS      10

typedef struct {
    int sock;
    char name[64];
    pthread_t thread;
    int is_conn;
    int room_no;    // 참여중인 채팅방 번호
} userinfo_t;

typedef struct {
    int   no;              // 방 번호 (1부터 시작)
    char  name[64];        // 방 이름 (개인방은 "user1-user2" 형태)
    int   is_private;      // 1: 개인채팅, 0: 공용채팅
    userinfo_t *members[MAX_USERS];
    int   member_count;
} chatroom_t;

static userinfo_t *users[MAX_USERS];
static chatroom_t  chatrooms[MAX_CHATROOMS];
static int client_count   = 0;
static int chatroom_count = 0;

// 사용자 목록 응답 함수
void resp_users(char *buf)
{
    buf[0] = '\0';
    for (int i = 0; i < client_count; i++) {
        if (users[i]->is_conn) {
            strcat(buf, users[i]->name);
            strcat(buf, ", ");
        }
    }
}

void show_userinfo(void)
{
    printf("%2s\t%16s\t%12s\t%s\n", "SD", "NAME", "CONNECTION", "ROOM");
    printf("========================================================\n");
    for (int i = 0; i < client_count; i++) {
        printf("%02d\t%16s\t%12s\t%02d\n",
            users[i]->sock,
            users[i]->name,
            users[i]->is_conn?"connected":"disconnected",
            users[i]->room_no
        );
    }
}

// 방 생성 (공용 or 개인)
int create_chatroom(const char *name, int is_private, userinfo_t **initial_members, int init_count)
{
    if (chatroom_count >= MAX_CHATROOMS) return -1;
    chatroom_t *r = &chatrooms[chatroom_count];
    r->no = chatroom_count + 1;
    strncpy(r->name, name, 63);
    r->name[63] = '\0';
    r->is_private = is_private;
    r->member_count = 0;
    for (int i = 0; i < init_count && i < MAX_USERS; i++) {
        r->members[r->member_count++] = initial_members[i];
    }
    chatroom_count++;
    return r->no;
}

void resp_chatrooms(char *buf)
{
    buf[0] = '\0';
    if (chatroom_count == 0) {
        strcpy(buf, "No chatrooms.\n");
        return;
    }
    for (int i = 0; i < chatroom_count; i++) {
        char line[128];
        snprintf(line, sizeof(line), "%d - %s %s\n",
            chatrooms[i].no,
            chatrooms[i].name,
            chatrooms[i].is_private?"(private)":"(public)");
        strcat(buf, line);
    }
}

// 특정 방 번호에 사용자 추가
void join_chatroom(userinfo_t *user, int room_no)
{
    for (int i = 0; i < chatroom_count; i++) {
        if (chatrooms[i].no == room_no) {
            chatroom_t *r = &chatrooms[i];
            if (r->member_count < MAX_USERS) {
                r->members[r->member_count++] = user;
                user->room_no = room_no;
            }
            break;
        }
    }
}

void leave_chatroom(userinfo_t *user)
{
    int room_no = user->room_no;
    if (room_no == ROOM0) return;
    for (int i = 0; i < chatroom_count; i++) {
        if (chatrooms[i].no == room_no) {
            chatroom_t *r = &chatrooms[i];
            // 멤버 목록에서 제거
            int idx = -1;
            for (int j = 0; j < r->member_count; j++) {
                if (r->members[j] == user) { idx = j; break; }
            }
            if (idx >= 0) {
                for (int j = idx; j < r->member_count-1; j++)
                    r->members[j] = r->members[j+1];
                r->member_count--;
            }
            break;
        }
    }
    user->room_no = ROOM0;
}

void broadcast_message(int room_no, const char *msg)
{
    if (room_no == ROOM0) {
        for (int i = 0; i < client_count; i++) {
            if (users[i]->is_conn && users[i]->room_no == ROOM0) {
                send(users[i]->sock, msg, strlen(msg), 0);
            }
        }
        return;
    }
    for (int i = 0; i < chatroom_count; i++) {
        if (chatrooms[i].no == room_no) {
            for (int j = 0; j < chatrooms[i].member_count; j++) {
                userinfo_t *u = chatrooms[i].members[j];
                if (u->is_conn) send(u->sock, msg, strlen(msg),0);
            }
            break;
        }
    }
}

void *client_process(void *arg)
{
    userinfo_t *user = (userinfo_t*)arg;
    char buf[512], out[512];
    int rb;

    send(user->sock, "Welcome! Input user name: ", 28,0);
    rb = recv(user->sock, buf, sizeof(buf)-1,0);
    buf[rb]=0; strtok(buf,"\n");
    strncpy(user->name, buf,63);
    user->is_conn = 1;
    user->room_no = ROOM0;

    while ((rb=recv(user->sock, buf, sizeof(buf)-1,0))>0) {
        buf[rb]=0; strtok(buf,"\n");
        char cmd_buf[512]; strcpy(cmd_buf,buf);
        char *cmd = strtok(cmd_buf," ");

        if (cmd && cmd[0]=='/') {
            if (!strcmp(cmd,"/users")) {
                char list[256]; resp_users(list);
                send(user->sock,list,strlen(list),0);
            }
            else if (!strcmp(cmd,"/new")) {
                char *rname = strtok(NULL,"");
                if (!rname) send(user->sock,"Usage: /new <name>\n",21,0);
                else {
                    int no = create_chatroom(rname,0,NULL,0);
                    snprintf(out,sizeof(out),"Chatroom '%s' created (#%d)\n",rname,no);
                    send(user->sock,out,strlen(out),0);
                }
            }
            else if (!strcmp(cmd,"/start")) {
                char *id = strtok(NULL," ");
                if (!id) continue;
                userinfo_t *peer=NULL;
                for (int i=0;i<client_count;i++) if(!strcmp(users[i]->name,id)) peer=users[i];
                if (peer) {
                    userinfo_t *init[2]={user,peer};
                    char pname[128]; snprintf(pname,128,"%s-%s",user->name,peer->name);
                    int no = create_chatroom(pname,1,init,2);
                    user->room_no = no; peer->room_no = no;
                    snprintf(out,sizeof(out),"Private chat created (#%d)\n",no);
                    send(user->sock,out,strlen(out),0);
                    send(peer->sock,out,strlen(out),0);
                }
            }
            else if (!strcmp(cmd,"/chats")) {
                char list[512]; resp_chatrooms(list);
                send(user->sock,list,strlen(list),0);
            }
            else if (!strcmp(cmd,"/enter")) {
                char *num_s=strtok(NULL," "); int no=num_s?atoi(num_s):0;
                if (no<=0||no>chatroom_count) send(user->sock,"Invalid room#\n",14,0);
                else {
                    join_chatroom(user,no);
                    snprintf(out,sizeof(out),"Entered room #%d\n",no);
                    send(user->sock,out,strlen(out),0);
                }
            }
            else if (!strcmp(cmd,"/exit")) {
                leave_chatroom(user);
                send(user->sock,"Exited room\n",11,0);
            }
            else if (!strcmp(cmd,"/dm")) {
                char *id=strtok(NULL," "); char *msg=strtok(NULL,"");
                if(!id||!msg) { send(user->sock,"Usage: /dm <user> <msg>\n",25,0); continue; }
                snprintf(out,sizeof(out),"[DM %s->%s] %s\n",user->name,id,msg);
                int found=0;
                for(int i=0;i<client_count;i++){
                    if(users[i]->is_conn && !strcmp(users[i]->name,id)){
                        send(users[i]->sock,out,strlen(out),0); found=1; break; }
                }
                if(!found) send(user->sock,"User not found\n",15,0);
            }
        }
        else {
            // 일반 메시지: 현재 방 번호 기준 브로드캐스트
            int no = user->room_no;
            snprintf(out,sizeof(out),"[%s] %s\n",user->name,buf);
            broadcast_message(no,out);
        }
    }
    user->is_conn=0;
    leave_chatroom(user);
    close(user->sock);
    return NULL;
}

int main(){
    int sd,ns,clilen=sizeof(struct sockaddr_in);
    struct sockaddr_in sin,cli;
    sd=socket(AF_INET,SOCK_STREAM,0);
    int opt=1; setsockopt(sd,SOL_SOCKET,SO_REUSEADDR,&opt,sizeof(opt));
    memset(&sin,0,sizeof(sin)); sin.sin_family=AF_INET; sin.sin_port=htons(PORTNUM);
    sin.sin_addr.s_addr=INADDR_ANY;
    bind(sd,(struct sockaddr*)&sin,sizeof(sin)); listen(sd,5);

    int epfd=epoll_create1(0);
    struct epoll_event ev,events[10];
    ev.events=EPOLLIN; ev.data.fd=sd; epoll_ctl(epfd,EPOLL_CTL_ADD,sd,&ev);
    ev.events=EPOLLIN; ev.data.fd=0;  epoll_ctl(epfd,EPOLL_CTL_ADD,0,&ev);
    printf("> "); fflush(stdout);
    while(1){
        int n=epoll_wait(epfd,events,10,-1);
        for(int i=0;i<n;i++){
            if(events[i].data.fd==sd){
                ns=accept(sd,(struct sockaddr*)&cli,&clilen);
                userinfo_t *u=calloc(1,sizeof(*u)); u->sock=ns;
                users[client_count++]=u;
                pthread_create(&u->thread,NULL,client_process,u);
            } else if(events[i].data.fd==0){
                char cmd[64]; fgets(cmd,sizeof(cmd),stdin);
                strtok(cmd," \n");
                if(!strcmp(cmd,"users")) show_userinfo();
                else if(!strcmp(cmd,"chats")){
                    char list[512]; resp_chatrooms(list); printf("%s",list);
                }
                printf("> "); fflush(stdout);
            }
        }
    }
    close(sd);
    return 0;
}
