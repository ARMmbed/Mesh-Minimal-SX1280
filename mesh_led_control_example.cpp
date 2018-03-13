/*
 * Copyright (c) 2016 ARM Limited. All rights reserved.
 * SPDX-License-Identifier: Apache-2.0
 * Licensed under the Apache License, Version 2.0 (the License); you may
 * not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an AS IS BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include "mbed.h"
#include "nanostack/socket_api.h"
#include "mesh_led_control_example.h"
#include "common_functions.h"
#include "ip6string.h"
#include "mbed-trace/mbed_trace.h"

static void init_socket();
static void handle_socket();
static void receiver_report();
static void receive();
static void my_button_isr();
static void send_message();
static void blink();
static void update_state(uint8_t state);
static void handle_message(char* msg);

#define multicast_addr_str "ff15::810a:64d1"
#define TRACE_GROUP "example"
#define UDP_PORT 1234
#define MESSAGE_WAIT_TIMEOUT (30.0)
#define MASTER_GROUP 0
#define MY_GROUP 1

extern Serial pc;
void scan_sdna(char buffer[]);
static void input_info();
static void packet_send_worker();

//DigitalOut output(A4, 1);
DigitalOut led_1(A5, 1);    // for the NXP new board testing
//DigitalOut led_1(MBED_CONF_APP_LED, 1);

InterruptIn my_button(SW3); // for the NXP new board testing
//InterruptIn my_button(MBED_CONF_APP_BUTTON);

NetworkInterface * network_if;
UDPSocket* my_socket;
// queue for sending messages from button press.
EventQueue queue;
// for LED blinking
Ticker ticker;
// Handle for delayed message send
int queue_handle = 0;

uint8_t multi_cast_addr[16] = {0};
uint8_t receive_buffer[256];

uint8_t destination_addr[16] = {0};
char destination_buffer[128];
char target_buffer[5];
char dummy_length[128];
int send_interbal=1;
int send_length=20;
int thread_flag=0;
// int send_try=20;
// int total_send_try=0;
long send_try=20;
long total_send_try=0;


long total_receive_try=10;
long receive_count=0;

int led_state =0;


int action_mode =0; // 0=receiver , 1=sender
// how many hops the multicast message can go
static const int16_t multicast_hops = 10;
bool button_status = 0;

void start_mesh_led_control_example(NetworkInterface * interface){
    //printf("start_mesh_led_control_example()\n");
    MBED_ASSERT(MBED_CONF_APP_LED != NC);
    MBED_ASSERT(MBED_CONF_APP_BUTTON != NC);

    network_if = interface;
    stoip6(multicast_addr_str, strlen(multicast_addr_str), multi_cast_addr);
    init_socket();
}

static void blink() {
    led_1 = !led_1;
}

void start_blinking() {
    ticker.attach(blink, 0.5);
}

void cancel_blinking() {
    ticker.detach();
    led_1=1;
}
static void packet_send_worker() {
    total_send_try++;
    char buf[256];
    int length;

    /**
    * Multicast control message is a NUL terminated string of semicolon separated
    * <field identifier>:<value> pairs.
    *
    * Light control message format:
    * t:lights;g:<group_id>;s:<1|0>;\0
    */
    length = snprintf(buf, sizeof(buf), "%10ld/%10ld:%s",total_send_try,send_try,dummy_length);
    MBED_ASSERT(length > 0);
    //printf("TX message, %u bytes: %s\n", length, buuf);
    printf(" seq : %10ld/%10ld ,", total_send_try,send_try);
    printf("len : %u , ", length);
    printf("interval : %d , ", send_interbal);
    printf("Tx to : %s \n", destination_buffer);
    SocketAddress send_sockAddr(destination_addr, NSAPI_IPv6, UDP_PORT);
    my_socket->sendto(send_sockAddr, buf, 50);
    
    if(total_send_try >= send_try){
        ticker.detach();
        total_send_try=0;
        thread_flag=0;
    }
    //After message is sent, it is received from the network
}


static void send_message() {
    //printf("send msg %d\n", button_status);

    

    stoip6(destination_buffer, strlen(destination_buffer), destination_addr);
    SocketAddress send_usi_sockAddr(destination_addr, NSAPI_IPv6, UDP_PORT);

    char buf[20];
    int length;

    /**
    * Multicast control message is a NUL terminated string of semicolon separated
    * <field identifier>:<value> pairs.
    *
    * Light control message format:
    * t:lights;g:<group_id>;s:<1|0>;\0
    */
    length = snprintf(buf, sizeof(buf), "t:lights;g:%03d;s:%s;", MY_GROUP, (button_status ? "1" : "0")) + 1;
    MBED_ASSERT(length > 0);
    printf("Sending lightcontrol message, %u bytes: %s\n", length, buf);
    SocketAddress send_sockAddr(multi_cast_addr, NSAPI_IPv6, UDP_PORT);
    //my_socket->sendto(send_sockAddr, buf, 20);
    my_socket->sendto(send_usi_sockAddr, buf, 20);
    //After message is sent, it is received from the network
}

// As this comes from isr, we cannot use printing or network functions directly from here.
static void input_info(){
    //printf("switch_3 active input info\n");
    printf("enter destinatino addr : \n");
    int flag=0;
    int index=0;

    char c;

    memset(destination_buffer, '\0', 128);


    index =  snprintf(destination_buffer, sizeof(destination_buffer), "fd00:db8::ff:fe00:");
    while(flag==0){
        c = pc.getc();
        pc.putc(c);
        destination_buffer[index]=c;
        target_buffer[index] = c;
        index++;
        if(c==13){flag=1;} //means enter key
        if(c==127){index = index-2;} // means back space key
    }
    flag=0; index=0;
    printf("\n");
    printf(" destinatino receivced : %s \n",destination_buffer);

    int length;
    char buf[20];

    length = snprintf(buf, sizeof(buf), "send ping : 20bytes.") + 1;
    stoip6(destination_buffer, strlen(destination_buffer), destination_addr);
    SocketAddress send_sockAddr(destination_addr, NSAPI_IPv6, UDP_PORT);


    char temp[10];
    printf("enter send length : 22 + ");
    scan_sdna(temp); 
    send_length = atoi(temp);
    printf("length : 23 + %d \n", send_length);
    memset(dummy_length, '\0', 128);
    for(;index <= send_length;index++){
        dummy_length[index] = 65;
    }
    memset(temp, '\0', 10);

    printf("enter send interbal : \n");
    scan_sdna(temp); 
    send_interbal = atoi(temp);
    printf("interbal : %d \n", send_interbal);
    memset(temp, '\0', 10);

    printf("enter goal send try : \n");
    scan_sdna(temp); 
    send_try = atol(temp);
    printf("send_try : %ld \n", send_try);
    memset(temp, '\0', 10);

    index=0;
    flag=0;
//    printf("unicast send end\n");
}

static void my_button_isr() {
    if(thread_flag==0){
        thread_flag=1;
        total_send_try=0;
        input_info();
        printf("\n\nSTART PING SEND\n\n");
        ticker.attach(packet_send_worker, send_interbal);
    }else{
        ticker.detach();
        total_send_try=0;
        thread_flag=0;
    }
    //button_status = !button_status;
}

static void update_state(uint8_t state) {
    if (state == 1) {
       printf("Turning led on\n\n");
       led_1 = 0;
       button_status=1;
       //output = 1;
       }
    else {
       printf("Turning led off\n\n");
       led_1 = 1;
       button_status=0;
       //output = 0;
   }
}

static void handle_message(char* msg) {
    // Check if this is lights message
    uint8_t state=button_status;
    uint16_t group=0xffff;

    if (strstr(msg, "t:lights;") == NULL) {
       return;
    }

    if (strstr(msg, "s:1;") != NULL) {
        state = 1;
    }
    else if (strstr(msg, "s:0;") != NULL) {
        state = 0;
    }

    // 0==master, 1==default group
    char *msg_ptr = strstr(msg, "g:");
    if (msg_ptr) {
        char *ptr;
        group = strtol(msg_ptr, &ptr, 10);
    }

    // in this example we only use one group
    if (group==MASTER_GROUP || group==MY_GROUP) {
        update_state(state);
    }
}
static void receiver_switch(){
    if(thread_flag==0){
        printf("report thread active\n");
        printf("Enter goal : \n");
        char temp[5];
        scan_sdna(temp);
        total_receive_try = atoi(temp);
        printf("goal : %ld \n", total_receive_try);
        thread_flag=1;
    }else{
         printf("report thread end\n");
        receiver_report();
        thread_flag=0;
        total_receive_try=0;
        receive_count=0;
    }
}

static void receiver_report(){
    float psr = (float)receive_count / (float)total_receive_try * 100.0;
    printf("\n\n\nEnd Receiver mode - Report \n");
    printf("  Goal count = %ld , receive = %ld  , successivity= %0.3f %%\n", total_receive_try, receive_count, psr);
}
static void receive_receiver(){
     // Read data from the socket
    // static long last_seq =0;
    SocketAddress source_addr;

    memset(receive_buffer, 0, sizeof(receive_buffer));
    bool something_in_socket=true;
    // read all messages
    while (something_in_socket) {
        int length = my_socket->recvfrom(&source_addr, receive_buffer, sizeof(receive_buffer) - 1);
        if (length > 0) {
            //int timeout_value = MESSAGE_WAIT_TIMEOUT;
                uint8_t temp[10];
                memcpy(temp, receive_buffer, 10);
                long now_seq = atol((char*)temp);
                // printf("now seq %ld, last_seq %ld \n", now_seq, last_seq);
            if( thread_flag==1){ //reporting
                receive_count++;              

                int len = strlen((char*)receive_buffer);
                printf("RX from %s, ", source_addr.get_ip_address());
                printf("len %d , ",len);  //why always 50???
                //printf("message: %s\n",receive_buffer);
                printf("conunt %ld , ",receive_count);
                printf("seq %.10s  , ", receive_buffer);


                // uint8_t* temp2[10];
                // memcpy(temp2, receive_buffer, 10);
                // long now_seq2 = atol((char*)temp2);
                float psr2 = (float)receive_count / (float) now_seq * 100.0;
                printf("psr %0.1f \n",psr2);
                // last_seq = now_seq;

                // float psr = (float)receive_count/(float)total_receive_try * 100.0;
                // printf("psr %0.1f %%  \n",psr); // 0.00....

                if(
                    receive_buffer[0] == receive_buffer[11] && // receivce_buffer[10] = "/"
                    receive_buffer[1] == receive_buffer[12] &&
                    receive_buffer[2] == receive_buffer[13] &&
                    receive_buffer[3] == receive_buffer[14] &&
                    receive_buffer[4] == receive_buffer[15] &&
                    receive_buffer[5] == receive_buffer[16] &&
                    receive_buffer[6] == receive_buffer[17] &&
                    receive_buffer[7] == receive_buffer[18] &&
                    receive_buffer[8] == receive_buffer[19] &&
                    receive_buffer[9] == receive_buffer[20]             
                    ){
                    printf("receive end packet report thread end\n");
                    receiver_report();
                    thread_flag=0;
                    total_receive_try=0;
                    receive_count=0;
                }


            }else{ // not reporting just print
                 printf("Packet from %s\n", source_addr.get_ip_address());
            }
        }
        else if (length!=NSAPI_ERROR_WOULD_BLOCK) {
            tr_error("Error happened when receiving %d\n", length);
            something_in_socket=false;
        }
        else {
            // there was nothing to read.
            something_in_socket=false;
        }
    }
}

static void receive() {
    // Read data from the socket
    SocketAddress source_addr;
    memset(receive_buffer, 0, sizeof(receive_buffer));
    bool something_in_socket=true;
    // read all messages
    while (something_in_socket) {
        int length = my_socket->recvfrom(&source_addr, receive_buffer, sizeof(receive_buffer) - 1);
        if (length > 0) {
            //int timeout_value = MESSAGE_WAIT_TIMEOUT;
            printf("Packet from %s\n", source_addr.get_ip_address());
            // timeout_value += rand() % 30;
            // printf("Advertisiment after %d seconds\n", timeout_value);
            // queue.cancel(queue_handle);
            // queue_handle = queue.call_in((timeout_value * 1000), send_message);
            // // Handle command - "on", "off"
            handle_message((char*)receive_buffer);
        }
        else if (length!=NSAPI_ERROR_WOULD_BLOCK) {
            tr_error("Error happened when receiving %d\n", length);
            something_in_socket=false;
        }
        else {
            // there was nothing to read.
            something_in_socket=false;
        }
    }
}
static void handle_socket_receiver() {
    // call-back might come from ISR
    queue.call(receive_receiver);
}

static void handle_socket() {
    // call-back might come from ISR
    queue.call(receive);
}

static void init_socket()
{
    char temp[5];
    printf("enter action mode : ");
    scan_sdna(temp); 
    action_mode = atoi(temp);
    printf("actino mode %d selected \n", action_mode);
    memset(temp, '\0', 5);

    my_socket = new UDPSocket(network_if);
    my_socket->set_blocking(false);
    my_socket->bind(UDP_PORT);
    my_socket->setsockopt(SOCKET_IPPROTO_IPV6, SOCKET_IPV6_MULTICAST_HOPS, &multicast_hops, sizeof(multicast_hops));

    ns_ipv6_mreq_t mreq;
    memcpy(mreq.ipv6mr_multiaddr, multi_cast_addr, 16);
    mreq.ipv6mr_interface = 0;

    my_socket->setsockopt(SOCKET_IPPROTO_IPV6, SOCKET_IPV6_JOIN_GROUP, &mreq, sizeof mreq);

    if(action_mode == 1 ){ // sender
        if (MBED_CONF_APP_BUTTON != NC) {
            my_button.fall(&my_button_isr);
            my_button.mode(PullUp);
        }
        //let's register the call-back function.
        //If something happens in socket (packets in or out), the call-back is called.
        my_socket->sigio(callback(handle_socket));
        my_button_isr();
    }else{  //receiver
            if (MBED_CONF_APP_BUTTON != NC) {
            my_button.fall(&receiver_switch);
            my_button.mode(PullUp);
        }
        my_socket->sigio(callback(handle_socket_receiver));
        receiver_switch();
    }
    

    // dispatch forever
    queue.dispatch();
}









void scan_sdna(char buffer[]){
    char c;
    //char buffer[128];
    int flag=0;
    int index=0;

     while(flag==0){
        c = pc.getc();
        pc.putc(c);
        buffer[index]=c;
        index++;
        if(c==13){flag=1;} //means enter key
        if(c==127){index = index-2;} // means back space key
    }
    flag=0; index=0;
    printf("\n");
    printf("eceivced : %s \n",buffer);
}
