#include<string>
#include<vector>
#include<iostream>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <sys/ioctl.h>
#include <sys/uio.h>
#include <net/if.h>

using namespace std;

string test = R"(Den ersten Bericht habe ich verfasst,
Theophilus, von allem was Jesus angefangen hat,
zu tun und auch zu lehren, bis zu dem Tag, an dem
er in den Himmel aufgenommen wurde, nachdem
er den Aposteln, die er sich auswerwählt, durch
den Heiligen Geist Befehl gegeben hatte. Diesen hat
er sich auch nach seinem Leiden in vielen sicheren
Kennzeichen lebendig dargestellt, indem er sich 
vierzig Tage hindurch von ihnen sehen ließ und über
die Dinge redete, die das Reich Gottes betreffen.

Jesu Himmelfahrt
Und als er mit ihnen versammelt war, befahl er 
ihnen, sich nicht von Jerusalem zu entfernen, sondern
auf die Verheißung des Vaters zu warten - die ihr,
sagte er, von mir gehört habt;
denn Johannes taufte mit Wasser, ihr aber werdet
mit Heiligem Geist getauft werden nach diesen wenigen
Tagen.
Sie nun, als sie zusammengekommen waren fragten
ihn und sagten: Herr, stellst du in dieser Zeit für
Israel das Reich wieder her? Er sprach zu ihnen: Es ist
nicht eure Sache, Zeiten oder Zeitpunkt zu wissen, die
der Vater in seiner eigenen Vollmacht festgesetzt hat.
Aber ihr werdet Kraft empfangen, wenn der 
Heilige Geist auf euch gekommen ist; und ihr werdet
meine Zeugen sein, sowohl in Jerusalem als auch in
ganz Judaea und Samaria und bis an das Ende der
Erde. Und als er dies gesgat hatte, wurde er vor ihren
Blicken emporgeheoben, und eine Wolke nahm ihn auf
vor ihren Augen weg.
Und als sie gespannt zum Himmel schauten, wie
er auffuhr, siehe da standen zwei Männer in weißen
Kleidern bei ihnen, die auch sprachen: Maenner
von Galilaea, was steht ihr und seht hinauf zum Himmel?
Dieser Jesus, der von wuch weg in den Himmel aufgenommen
worden ist, wird so kommen, wie ihr ihn habt hingehen 
sehen in den Himmel. Da kehrten sie nach Jerusalem zurueck
von dem Berg, welcher Oelberg heißt, der nahe bei Jerusalem ist,
einen Sabbatweg entfernt.
Und als sie hineingekommen waren, stiegen sie hinauf
in den Obersaal, wo sie scih aufzuhalten pflegten:
sowohl Petrus als Johannes und Jokobus und Matthäus,
Jakobus, der Sohn das Alphaeus, und Simon, der
Eiferer, und Judas, der Sohn des Jakobus. Diese
alle verharrten einmuetig im Gebet mit einigen
Frauen und Maria, der Mutter Jesu, und mit seinen
Bruedern.)";


int create_and_bind_raw_can_socket(std::string can_id){
 int s;
 sockaddr_can addr;
 ifreq ifr;
 if((s = socket(PF_CAN, SOCK_RAW, CAN_RAW)) < 0)
  throw string{"socket call failed"};
 strcpy(ifr.ifr_name, can_id.c_str());
 if (-1 == ioctl(s, SIOCGIFINDEX, &ifr))
  throw string{"socket call failed"};
 addr.can_family  = AF_CAN;
 addr.can_ifindex = ifr.ifr_ifindex;
 if(bind(s, (struct sockaddr *)&addr, sizeof(addr)) < 0)
  throw string{"bind call failed"};
 return s;
}

void send_can_frame(int s, std::uint8_t* data, size_t len, canid_t id){
 can_frame frame;
 memset(&frame,0,sizeof(frame));
 memcpy(frame.data,(char*)data,std::min((int)len,CAN_MAX_DLEN));
 frame.can_id = id;
 frame.can_dlc = ::min((int)len,CAN_MAX_DLEN);
 auto nbytes = write(s, &frame, sizeof(frame));
 if (nbytes != sizeof(frame))
	 throw string{"write failed"};
}

struct Can{
    int h{-1};
    static const int string_id_base{45};
    static const int string_id_announce{0};
    static const int string_id_fragment{1};

    static const unsigned short max_payload_string = 4;
    static const unsigned short max_payload_can = 8;

    Can() = default;
    Can(string can_id){
        h = create_and_bind_raw_can_socket(can_id);
    }

    void announce_string(string s){
        unsigned int len = s.length();
        send_can_frame(h,
                       reinterpret_cast<uint8_t*> (&len), 
                       sizeof(len), 
                       string_id_base + string_id_announce);
    }

    void send_string_partial(uint32_t offset, string s){
        uint8_t buffer[max_payload_can] = {};
        memcpy(buffer,&offset,sizeof(offset));
        memcpy(buffer+sizeof(offset),s.c_str(), ::min(s.length(),(size_t)max_payload_string) );
        send_can_frame(h,
                       buffer, 
                       sizeof(buffer), 
                       string_id_base + string_id_fragment);
    }
};

Can operator << (Can can, string v){
    can.announce_string(v);
    for (uint32_t pos = 0; pos < v.length(); pos+=Can::max_payload_string){
        auto s{v.substr(pos,Can::max_payload_string)};
        can.send_string_partial(pos,s);
    }
    return can;
} 

int main(){
    try{
        Can can{"vcan0"};
        can << test;
    } catch (string e)
    {
        cerr << e << endl;
    }
}






















