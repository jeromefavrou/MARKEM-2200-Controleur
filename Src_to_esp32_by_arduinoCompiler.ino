/*!BSD 3-Clause License

Copyright (c) 2024, Favrou jérôme

Redistribution and use in source and binary forms, with or without
modification, are permitted provided that the following conditions are met:

1. Redistributions of source code must retain the above copyright notice, this
   list of conditions and the following disclaimer.

2. Redistributions in binary form must reproduce the above copyright notice,
   this list of conditions and the following disclaimer in the documentation
   and/or other materials provided with the distribution.

3. Neither the name of the copyright holder nor the names of its
   contributors may be used to endorse or promote products derived from
   this software without specific prior written permission.

THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.*/

//#define DEBUG_MODE



#define AP_SSID "Controleur2200Markem"  // Définir un ssid
#define AP_PSWD "maintenance"  // Définir un mot de passe 8 caractere mini
#define DNS "controleur2200markem" // definir un nom de domaine


//! Attention version soft wifi 
#include <WiFi.h>
#include <WiFiMulti.h>
#include <WiFiAP.h>
#include <ESPmDNS.h>
#include <WiFiServer.h>

#ifndef __ASSERT_USE_STDERR
    #define __ASSERT_USE_STDERR
#endif
  
#include <assert.h>

/*
   _____ _                        _ _____  _        
  / ____| |                      | |  __ \| |       
 | (___ | |__   __ _ _ __ ___  __| | |__) | |_ _ __ 
  \___ \| '_ \ / _` | '__/ _ \/ _` |  ___/| __| '__|
  ____) | | | | (_| | | |  __/ (_| | |    | |_| |   
 |_____/|_| |_|\__,_|_|  \___|\__,_|_|     \__|_|   
                                                    
                                                    
    memory managemnt
    manage a shared pointers
    makes the use of pointers safe
    pointer managed even when copying SharedPtr
*/
template<typename T> class SharedPtr 
{
    protected : 
        // shared ptr
        T * m_ptr;

        //number of pointers to the same variable
        short * m_count;

        void (*m_destruct)(T *);

    public:
        SharedPtr(void):m_ptr(nullptr),m_count(new short(0))
        {
            this->m_destruct =  NULL;
        }

        //constructor with pointer passing
        SharedPtr(T * _ctr):m_ptr(_ctr),m_count(new short(1))
        {
            this->m_destruct =  NULL;

            if(_ctr == NULL)
                *m_count=0;
        }

         //constructor with pointer passing with destructor function
        SharedPtr(T * _ctr , void (*destruct)(T *) ):m_ptr(_ctr),m_count(new short(1))
        {
            this->m_destruct=destruct;

            if(_ctr == NULL)
                *m_count=0;
        }

        //constructor with raw variable passing
        SharedPtr(T & _ctr):m_ptr(&_ctr),m_count(new short(1))
        {

            this->m_destruct =  NULL;
        }

        //share constructor
        SharedPtr(SharedPtr<T> const & _trm):m_ptr(_trm.m_ptr)
        {
            //share counter
            this->m_count=_trm.m_count;

            //update counter
            ++*this->m_count;
        }

        //copy constructor
        SharedPtr operator=(SharedPtr<T> const & _trm)
        {
            //free actual ptr
            this->free();

            //share counter
            this->m_count= _trm.m_count;
            //update counter
            ++*this->m_count;

            //shared ptr
            this->m_ptr= _trm.m_ptr;

            return *this;
        }

        

        //access to give pointer
        virtual T * operator->()
        {
            //check if ptr is null and else throw an assert
            assert(!this->isNull());

            return this->m_ptr;
        }

        //return the counter
        int getCount(void) const 
        {
            return *this->m_count;
        }

        //returns the shared pointer without allowing its modification
        T * get(void) const 
        {
            return this->m_ptr;
        }

        //check if ptr is null
        inline bool isNull(void) const
        {
            return this->m_ptr == NULL;
        }

        void free(void)
        {
            if(this->isNull())
                return;
            //if last shared pointer
            //free memory
            if(*this->m_count == 1)
            {
                if(this->m_destruct !=  NULL)
                    this->m_destruct(this->m_ptr);
                else
                    delete this->m_ptr;

                delete this->m_count;
                
                this->m_ptr = nullptr;
                return;
            }

            //uncounting pointer to the counter
            --*this->m_count;
        }

        

        //destructor
        virtual ~SharedPtr()
        {    
            //if last shared pointer
            //free memory
            this->free();
        }

};



/*
reponse http OK
*/
void sendOk(WiFiClient & client)
{
    client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println();
}

/*
envoie du pied de page html
*/
void sendFooter(WiFiClient & client)
{
    client.print(F("<footer style=\"color:red;font-size:10px;\">"
    "<p>veiller à alimenter en 230v l'alimentation markem 2200 que par le cordon du boitier de contrôle </p>"
  "<p>en cas de doute sur des valeurs , utiliser un multimetre </p>"
  "<p>les valeurs peuvent dépendre de la température ambiante</p>"
    "<p>vous restez seul juge de votre diagnostic</p>"
  "<p>ce boitier de contrôle n'est pas un multimètre , les valeurs affiché sont précise au 10eme </p>"
    "<p>ne pas utiliser ce boitier de contrôle pour un autre usage que celui des imprimant markem 2200 </p>"
    "<p>l'appui sur lancer un test vous engage comme responsable de l'utilisation faite</p>"
  
"</footer>"));
}
/*
envoie des message de msie en garde html
*/
void mise_en_garde(WiFiClient & client)
{
    client.print(F("<div class=\"cptShwo\" style=\"color:red;font-size:11px;\">"
    "<p>veiller à alimenter en 230v l'alimentation markem 2200 que par le cordon du boitier de contrôle </p>"
  "<p>en cas de doute sur des valeurs , utiliser un multimetre </p>"
  "<p>les valeurs peuvent dépendre de la température ambiante</p>"
    "<p>vous restez seul juge de votre diagnostic</p>"
  "<p>ce boitier de contrôle n'est pas un multimètre , les valeurs affiché sont précise au 10eme </p>"
    "<p>ne pas utiliser ce boitier de contrôle pour un autre usage que celui des imprimant markem 2200 </p>"
    "<p>l'appui sur lancer un test vous engage comme responsable de l'utilisation faite</p>"
  
"</div>"));
}
/*
envoie du bouton de lancer de test html
*/
void sendUpdateButton (WiFiClient & client)
{
    client.print(F("<form action=\"/\" method=\"get\">" 
            "<div class=\"button\">"
            "<button type=\"submit\">Lancer un test</button>"
            "</div>"
            "</form>"
            ));
}
/*
envoie du bloc message d'avertissemnt html
*/
void sendAdvertisse(WiFiClient & client , String const & msg)
{
    client.print(
            "<div class=\"cptShwo\">"
            "<p>"+String(msg)+"</p>"
            " </div>"
 
            );
}

/*
code generique d'afffichege d'un ensemble de valeur 
envoie du code html
*/
template<int N> void send_showValue(WiFiClient &client, float value[N], float attempt, float min, float max, String const &title, String const &name = "", String const &unit = "", String const &help = "")
{
    // Fonction d'échappement des caractères HTML
    auto escapeHtml = [](const String& str) -> String {
        String escaped;
        for (char c : str) {
            switch (c) {
                case '&': escaped += "&amp;"; break;
                case '<': escaped += "&lt;"; break;
                case '>': escaped += "&gt;"; break;
                case '"': escaped += "&quot;"; break;
                case '\'': escaped += "&#39;"; break;
                default: escaped += c; break;
            }
        }
        return escaped;
    };

    // Utilisation de la fonction d'échappement
    String escapedTitle = escapeHtml(title);
    String escapedName = escapeHtml(name);
    String escapedUnit = escapeHtml(unit);
    String escapedHelp = escapeHtml(help);

    // Début du div
    client.print("<div class=\"incrust\">");
    client.print("<h4>" + escapedTitle + "</h4>");

    // Affichage des valeurs
    for (int i = 0; i < N; i++) {
        String valueClass = (value[i] > max || value[i] < min) ? "valueNOk" : "valueOk";
        client.print("<div class=\"element\"><h5>" + escapedName + String(i) + "</h5>");
        client.print("<div class=\"" + valueClass + "\">" + String(value[i]) + " " + escapedUnit + "</div></div>");
    }

    // Affichage des informations supplémentaires
    client.print("<div><h5>Attendu</h5><div class=\"value\">" + String(attempt) + " " + escapedUnit + "</div></div>");
    client.print("<p style=\"color:blue;font-size:10px;\">Info: " + escapedHelp + "</p></div>");
}

/*
envoie de l'entete html
*/
void sendHeader(WiFiClient & client)
{
    client.print(F("<header>"
  "<title>Tester Alimentation moteur pas a pas Markem IMAJE 2200</title>"
  "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\"/>"
  "<meta name=\"author\" content=\"Favrou Jerome ( SYSJF compagny SIRET : 89206560800010)\">"
    "<style>"
      "form,.cptShwo {"
      "background-color: white;"
    "margin: 0 auto;"
    "width: 50%;"

    "padding: 1em;"
    "border: 1px solid #CCC;"
    "border-radius: 1em;"
   " margin-top: 10px;}"
   

  

  "form div + div {margin-top: 1em;}"


  ".cptValue {"

  "width:60%;"
   "border-color: black;"
    "border-radius:4px;"
    "margin:4px 0;"
    "outline:none;"
    "padding:4px;"
    "box-sizing:border-box;"
    "transition:.3s;}"
  

    ".incrust {"
    "text-align: center;"
     " background-color: white;"
    "margin: 0 auto;"
    "width: 50%;"

    "padding: 1em;"
   " border: 1px solid #CCC;"
   " border-radius: 1em;"
    "margin-top: 10px;}"

  ".value {"
     "border: 2px solid #CCC;"
     "border-radius: 6px;"
     "margin: 5px ;"

     "padding-left: 10px;"
     "padding-right: 10px;"
     "padding-top: 5px;"
     "padding-bottom: 5px;}"

  ".valueOk{"
     "border: 2px solid #CCC;"
     "border-radius: 6px;"
    " margin: 5px ;"

     "padding-left: 10px;"
     "padding-right: 10px;"
     "padding-top: 5px;"
     "padding-bottom: 5px;"

     "background-color: green;  }"

  ".valueNOk{"
     "border: 2px solid #CCC;"
     "border-radius: 6px;"
     "margin: 5px ;"

     "padding-left: 10px;"
     "padding-right: 10px;"
     "padding-top: 5px;"
     "padding-bottom: 5px;"

     "background-color: red;}"

  ".element{display: inline-block;}"
  "h5,h4{margin: 5px;padding: 0px;}"
  "p{text-align: left;}"


  ".button {padding-left: 90px; }"

    
  "button {"

      "background-image: linear-gradient(to right, #D38312 0%, #A83279  51%, #D38312  100%);"
            "margin: 10px;"
            "padding: 15px 45px;"
            "text-align: center;"
            "text-transform: uppercase;"
            "transition: 0.5s;"
            "background-size: 200% auto;"
            "color: white;    "
            "box-shadow: 0 0 20px #eee;"
            "border-radius: 10px;"
            "display: block;}"
  
  "button:hover {"
            "background-position: right center; "
            "color: #fff;"
           " text-decoration: none; }"
         
  "footer {"
    "font-size: small;"
     "margin : 0 auto;"
  "padding: 1em;}"
  
    "body {"
     "background-color:grey;}"
  

  "</style>"
    "</header>"));
}

/*
fonction de décodage caractere speciaux
   */
void htlmDecodeSpecialChar(String & data)
{
  String const encodeChar[225]= {" ","!","\"","#","$","%","&","\'","(",")","*","+",",","-",".","/","0","1",
              "2","3","4","5","6","7","8","9",":",";","<","=",">","?","@","A",
              "B","C","D","E","F","G","H","I","J","K","L","M","N","O","P","Q","R","S","T","U","V","W","X","Y","Z",
              "["," ","]","^","_","`","a","b","c","d","e","f","g","h","i","j","k","l","m","n","o","p","q","r","s",
              "t","u","v","w","x","y","z","{","|","}","~"," ","€","","‚","ƒ","„","…","†","‡","ˆ","‰","Š","‹","Œ","","Ž","","","‘","’","“","”",
              "•","–","—","˜","™","š","›","œ","","ž","Ÿ"," ","¡","¢","£","¤","¥","¦","§","¨","©","ª","«","¬","­","®","¯",
              "°","±","²","³","´","µ","¶","·","¸","¹","º","»","¼","½","¾","¿","À","Á","Â","Ã","Ä","Å","Æ","Ç","È",
              "É","Ê","Ë","Ì","Í","Î","Ï","Ð","Ñ","Ò","Ó","Ô","Õ","Ö","×","Ø","Ù","Ú","Û","Ü","Ý","Þ","ß","à","á","â","ã","ä","å","æ",
              "ç","è","é","ê","ë","ì","í","î","ï","ð","ñ","ò","ó","ô","õ","ö","÷","ø","ù","ú","û","ü","ý","þ","ÿ"};

  for( int i = 0 ; i < 225 ; i++)
  {
    String encodeSrch = "%";
    encodeSrch += String(i+32 , HEX);
    encodeSrch.toUpperCase();

    data.replace(encodeSrch , encodeChar[i]);
  }
}

#ifdef DEBUG_MODE
/*
fonction de définition de débugage evenement wifi
   */
static void debugWiFiEvent(WiFiEvent_t event)
{
     switch (event) {
        case ARDUINO_EVENT_WIFI_READY: 
            Serial.println("WiFi interface ready");
            break;
        case ARDUINO_EVENT_WIFI_SCAN_DONE:
            Serial.println("Completed scan for access points");
            break;
        case ARDUINO_EVENT_WIFI_STA_START:
            Serial.println("WiFi client started");
            break;
        case ARDUINO_EVENT_WIFI_STA_STOP:
            Serial.println("WiFi clients stopped");
            break;
        case ARDUINO_EVENT_WIFI_STA_CONNECTED:
            Serial.println("Connected to access point");
            break;
        case ARDUINO_EVENT_WIFI_STA_DISCONNECTED:
            Serial.println("Disconnected from WiFi access point");
            break;
        case ARDUINO_EVENT_WIFI_STA_AUTHMODE_CHANGE:
            Serial.println("Authentication mode of access point has changed");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP:
            Serial.print("Obtained IP address: ");
            Serial.println(WiFi.localIP());
            break;
        case ARDUINO_EVENT_WIFI_STA_LOST_IP:
            Serial.println("Lost IP address and IP address is reset to 0");
            break;
        case ARDUINO_EVENT_WPS_ER_SUCCESS:
            Serial.println("WiFi Protected Setup (WPS): succeeded in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_FAILED:
            Serial.println("WiFi Protected Setup (WPS): failed in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_TIMEOUT:
            Serial.println("WiFi Protected Setup (WPS): timeout in enrollee mode");
            break;
        case ARDUINO_EVENT_WPS_ER_PIN:
            Serial.println("WiFi Protected Setup (WPS): pin code in enrollee mode");
            break;
        case ARDUINO_EVENT_WIFI_AP_START:
            Serial.println("WiFi access point started");
            break;
        case ARDUINO_EVENT_WIFI_AP_STOP:
            Serial.println("WiFi access point  stopped");
            break;
        case ARDUINO_EVENT_WIFI_AP_STACONNECTED:
            Serial.println("Client connected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STADISCONNECTED:
            Serial.println("Client disconnected");
            break;
        case ARDUINO_EVENT_WIFI_AP_STAIPASSIGNED:
            Serial.println("Assigned IP address to client");
            break;
        case ARDUINO_EVENT_WIFI_AP_PROBEREQRECVED:
            Serial.println("Received probe request");
            break;
        case ARDUINO_EVENT_WIFI_AP_GOT_IP6:
            Serial.println("AP IPv6 is preferred");
            break;
        case ARDUINO_EVENT_WIFI_STA_GOT_IP6:
            Serial.println("STA IPv6 is preferred");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP6:
            Serial.println("Ethernet IPv6 is preferred");
            break;
        case ARDUINO_EVENT_ETH_START:
            Serial.println("Ethernet started");
            break;
        case ARDUINO_EVENT_ETH_STOP:
            Serial.println("Ethernet stopped");
            break;
        case ARDUINO_EVENT_ETH_CONNECTED:
            Serial.println("Ethernet connected");
            break;
        case ARDUINO_EVENT_ETH_DISCONNECTED:
            Serial.println("Ethernet disconnected");
            break;
        case ARDUINO_EVENT_ETH_GOT_IP:
            Serial.println("Obtained IP address");
            break;
        default: break;
    }

  }
#endif

#define FactorcalibrationAnalog 1.04f
#define OffsetcalibrationAnalog 0.10f
#define Vref 3.3f
#define ADC_reso 4095.0f
#define Echentillonage 20

/*
   lecture analogique par moyennage et calibration
*/
template<uint8_t n> float analogReader(uint8_t pin )
{
  uint8_t nread =  n == 0 ? 1 : n;

  float res = 0;

   //lecture de stabilisation
  analogRead(pin);
  delay(1);

   
  // integration des N valeur lu
   
  for( auto i =0 ; i < nread ; i++)
  {
    res+=analogRead(pin);
    delay(1);
  }
    
// moyenne de la population de valeur
  res /= (float)nread ;

  #ifdef DEBUG_MODE
  Serial.print(F("raw analog : "));
  Serial.println(res);
  #endif

   //calibration 
  res =OffsetcalibrationAnalog + FactorcalibrationAnalog * res * (Vref/ADC_reso);

   //mappage des extremité high et low
  if( res > Vref)
    res = Vref;
  if(res  <= OffsetcalibrationAnalog)
    res = 0;

  #ifdef DEBUG_MODE
  Serial.print(F("calibrate analog : "));
  Serial.println(res);
  #endif
  
  return res;
}

/*
   convertion d'une valeur analogique de sortie pont diviseur en tension d'entrée pont diviseur
   */
template<unsigned int r1 , unsigned int r2> float bridgeDivisorU1( float vout  )
{
  if( r2 ==0 ||  (r1 + r2 )==0)
    return 0;

  return   vout / ( (float)r2 / (r1 + r2 )) ;
}
/*
   convertion d'une valeur analogique de sortie pont diviseur et d'une valeur d'entrée pont diviseur en valeur ohmique des la resistance de tete de pont
   */
template<unsigned int r1>  float bridgeDivisorR2( float ui ,float vout )
{
  if(ui <= 1e-6 || vout <= OffsetcalibrationAnalog || ui - vout <= OffsetcalibrationAnalog   )
    return 0;

  return vout * (float)r1 / (  ui - vout) ;
}

/*
 __          ___  __ _  _____           _   
 \ \        / (_)/ _(_)/ ____|         | |  
  \ \  /\  / / _| |_ _| |  __  ___  ___| |_ 
   \ \/  \/ / | |  _| | | |_ |/ _ \/ __| __|
    \  /\  /  | | | | | |__| |  __/\__ \ |_ 
     \/  \/   |_|_| |_|\_____|\___||___/\__|
   */
//class de gestion des evenement lier au wifi
class WifiGest
{
  public:

  struct connectId
  {
    unsigned int ssid , pswd;
  };

    WifiGest(String const & _ap_ssid , String const & _ap_pswd , String const & _dns );

    SharedPtr<WiFiClass> & get_wifi_inst(void);
    
    void server_handle(uint8_t pin_A[4] , uint8_t pin4 , uint8_t pin5 , uint8_t relais1 , uint8_t relais2 ,uint8_t relais3);
    void stop(void);
  protected : 
    SharedPtr<WiFiClass> m_wifi;
    SharedPtr<WiFiServer> m_server;
    

    String m_ap_ssid,m_ap_pswd, m_dns;

  private :

  void dataRuntime(SharedPtr<String> & _data);
  String getRoot(String const & _data);
};

WifiGest::WifiGest(String const & _ap_ssid , String const & _ap_pswd , String const & _dns  ):m_ap_ssid(_ap_ssid),m_ap_pswd(_ap_pswd),m_dns(_dns)
{
  this->m_wifi = SharedPtr<WiFiClass>( new WiFiClass() ) ;

   //definition du server tcp ouvert en port 80
  this->m_server = SharedPtr<WiFiServer>( new WiFiServer(80) ) ;

  #ifdef DEBUG_MODE
    this->m_wifi->onEvent( debugWiFiEvent );
  #endif

   // init en mode sta et definition du DNS
  this->m_wifi->mode(WIFI_OFF);
  this->m_wifi->mode(WIFI_AP_STA);
  this->m_wifi->setHostname( this->m_dns.c_str());

   //si echec coupure du module wifi
  if( !this->m_wifi->softAP( this->m_ap_ssid.c_str() , this->m_ap_pswd.c_str() ) )
  {
  this->m_wifi->mode(WIFI_OFF);
  return;
  }
  
   //inti de l'ip local
  if(!this->m_wifi->softAPConfig(IPAddress(192,168,2,1) , IPAddress(192,168,2,1) , IPAddress(255,255,255,0)))
  {
  this->m_wifi->mode(WIFI_OFF);
  return ;
  }

   //demarage service dns
  MDNS.begin( this->m_dns.c_str() );

   //démarage serveur tcp
  this->m_server->begin();
        

}

void WifiGest::stop(void)
{
   //arret du wifi
  this->m_wifi->mode(WIFI_OFF); 
}
SharedPtr<WiFiClass> & WifiGest::get_wifi_inst(void)
{
   //retourn l'objet de gestion interne wifi
  return this->m_wifi;
}

/*
methode de boucle principal serveur "min"
   */
void WifiGest::server_handle(uint8_t pin_A[4] , uint8_t pin4 , uint8_t pin5 , uint8_t relais1 , uint8_t relais2 ,uint8_t relais3)
{

  WiFiClient client = this->m_server->available();  
   //attente d'un client
  if (client)
  {       

    SharedPtr<String> resp = SharedPtr<String>(new String(""));

   //lecture datagrame TCP
    while (client.connected())
    {
      delay(500);
      while (client.available())           
        *resp.get() +=(char) client.read();
      if(!client.available())
        break;           
    }
    
    #ifdef DEBUG_MODE
      Serial.println(*resp.get());  
    #endif

   //attente mini get ou post 
    if(resp->length()>=3 )
    {
      // si requette http get
      if( resp->substring( 0, 3 ) == "GET" )
      {
        String root = this->getRoot(resp->substring( 4, 24 ));

        #ifdef DEBUG_MODE
          Serial.println( String(" Chemin : ") + root);  
        #endif

         //si racine demandé envoie de la page d'aceuil et d'avertissement (html)
        if(  root == "/")
        {
          sendOk(client);
          client.print( "<html> ");
          client.print("<meta http-equiv=\"refresh\" content=\"10\" >");
          
          sendHeader(client);
          
          mise_en_garde(client);

         // vérification presence alim markem2200 ( 36v en tete de pont)
          bridgeDivisorU1< 20000 , 1000 >(analogReader<Echentillonage>(pin5) );
          delay(500);

          if(  bridgeDivisorU1< 20000 , 1000 >(analogReader<Echentillonage>(pin5) ) <= 2.18 )
            sendAdvertisse(client , "Alimentation non détectée : Vérifier que l alimentation est bien branchée et allumée" );

          // regarde si cable moteur branché

          float  res[4];

           //verification présence moteur bipolaire

          for(auto i =0 ; i < 4 ; i++)
          {
            res[i] = analogReader<Echentillonage>(pin_A[i]);
          }

          if( res[0]  >= 1 && res[1] >= 1 && res[2] >= 1 && res[3] >= 1)
          {
            sendAdvertisse(client , "Moteur bipolaire non détecter : Vérifier que le connecteur est bien branché" );
          }

          sendUpdateButton(client);
                    
          sendFooter(client);
          
          client.print( " </html> ");
          client.println();
        }
           //si requette de test recu
        else if( root == "/?" )
        {
         
          sendOk(client);

          client.print( "<html> ");
          
          sendHeader(client);

          sendUpdateButton(client);

          float uref[1] ;
          delay(100);
          
          //prise de la tension de référence boitier ( 24vcc en tete d epont)
          uref[0] = bridgeDivisorU1< 10000 , 1000 >(analogReader<Echentillonage>(pin4));
          if( uref[0] < 1.72)
            uref[0] = 0;


          #ifdef DEBUG_MODE
            Serial.print(F("tension de référence: "));
            Serial.println(uref[0]);
          #endif

         // envoie du bloc html d'affichage de la valeur
          send_showValue<1>( client , uref , 24 , 18, 29 , "Tension de référence" , "U" , "V" , "Référence pour certaines mesures" );
          
          float Ualim[1] ;
          //controle tension 36v
          Ualim[0] = bridgeDivisorU1< 20000 , 1000 >(analogReader<Echentillonage>(pin5) );
          if( Ualim[0] < 2.81)
            Ualim[0] = 0;

          #ifdef DEBUG_MODE
            Serial.print(F("tension alim: "));
            Serial.println(Ualim[0]);
          #endif

           // envoie du bloc html d'affichage de la valeur
          send_showValue<1>( client , Ualim , 36 , 32, 40 , "1 - CONTROLE TENSION ALIMENTATION" , "U" , "V" , "Des tensions trop faibles peuvent amener un mauvais fonctionnement ( vibration, ... ) et des tensions > à 44v sont destructives");
          

          //controle presence moteur
          float  res[4];

          for(auto i = 0 ; i < 4 ; i++)
          {
            res[i]=analogReader<Echentillonage>(pin_A[i]);

            #ifdef DEBUG_MODE
              Serial.print(F("cont: "));
              Serial.println(res[i]);
            #endif
          }

          send_showValue<4>(client , res , 0 , 0.0f, 0.2f ,"2 - CONTROLE PRESENCE ENROULEMENT MOTEUR BIPOLAIRE", "CONT", "V","on attend des valeurs très proche de 0v , pour valider la continuité d'un enroulement" );
          //coupure 230v et switch tension de controle enroulement
          digitalWrite(relais2 , LOW );

          sendAdvertisse(client , "3 - COUPURE 230V ALIMENTATION" );

          delay(1000);

           //si tout les enroulement détecter
          
          if( res[0] < 0.2 &&  res[1] < 0.2 && res[2] < 0.2 && res[3] < 0.2)
          {
            // envoie du 24vcc pour controle enroulmeent
            digitalWrite(relais3 , LOW );

            delay(2000);

             // mesure des enrouelment
            for(auto i = 0 ; i < 4 ; i++)
            {
              res[i] = bridgeDivisorR2<50>( uref[0] , analogReader<Echentillonage>(pin_A[i]) );

              #ifdef DEBUG_MODE
                Serial.print(F("res: "));
                Serial.println(res[i]);
              #endif
            }

            send_showValue<4>( client , res , 2 , 1.3, 2.6 , "4 - CONTROLE RESISTANCES ENROULEMENTS MOTEUR BIPOLAIRE" , "R" , "Ω" , "Des résistances trop fortes peuvent amener un mauvais fonctionnement ( vibration, ... ) et des résistance < a 1Ω sont destructive" );

            //coupure du 24v et repassage sur 3.3v
            digitalWrite(relais3 , HIGH );
          }
          else
          {
            sendAdvertisse(client , "Des enroulements n'ont pas été vus , mesures des résistances non exécutées" );
          }

          delay(1000);
           
          sendAdvertisse(client , "5 - CONTROLE TENSION ALIMENTATION NUL  patienter ... +-30s" );

           //attente d'une tension nulle a la place de 36vcc
          long int i = millis();
          while(bridgeDivisorU1< 20000 , 1000 >(analogReader<Echentillonage>(pin5) ) > 2.18)
          {
            #ifdef DEBUG_MODE
                Serial.print(F("u alim: "));
                Serial.println(bridgeDivisorU1< 20000 , 1000 >(analogReader<Echentillonage>(pin5) ));
            #endif
            if( millis() - i >=30000)
              break;
            delay(50);
            
            i=millis();
          }
          
            
          if( millis() - i <= 30000)
          {
            //mesure des diode mosfet
            if(bridgeDivisorU1< 20000 , 1000 >(analogReader<Echentillonage>(pin5)) <= 2.18 )
            {
               //switch des pont diviseur , passage de 50 a 5k
              digitalWrite(relais1 , LOW);
              delay(10000);

              for(auto i=0 ; i < 4;i++)
              {
                res[i]= (Vref -analogReader<Echentillonage>(pin_A[i]))*1000;

                #ifdef DEBUG_MODE
                  Serial.print(F("u(d) diode: "));
                  Serial.println(res[i]);
                #endif
              }

               //repassage sur les 50ohm
              digitalWrite(relais1 , HIGH);

               //evoie des valeur dans le bloc d'affichage html
              send_showValue<4>( client , res , 600 , 300, 900 , "6 - CONTROLE DES MOSFET PILOTAGE MOTEUR BIPOLAIRE" , "ΔU" , "mV" , "Les valeurs affichées sont plus dures à interpréter. Elles représentent la chute de tension aux bornes du MOSFET, une diode de roue libre y est presente, c'est elle qui provoque la chute de tension, des valeurs  proches de 0 mv indique un MOSFET ( HS)  passant ou shunté par sa diode passant (HS) .  Dès valeur proche de 3300 mv indiqueraient une diode coupée et un MOSFET non passant ou bien une connectique non branché. Des valeurs correcte ne garantissent pas que le MOSFET est fonctionnel il peut etre jamais passant");

              delay(500);
            }
            else
            {
              sendAdvertisse(client , "Tension alimentation non nulle , mesures des MOSFET non exécutées" );
            }
          }
          else
          {
            sendAdvertisse(client , "Tension alimentation non nulle , mesures des MOSFET non exécuteées" );
          }

           //garentie de etat repos relais
          digitalWrite(relais3 , HIGH );
          digitalWrite(relais1 , HIGH );
          digitalWrite(relais2 , HIGH );

          sendAdvertisse(client , "CONTROLE TERMINER" );
                    
          sendFooter(client);
          
          // The HTTP response ends with another blank line:
          client.print( " </html> ");
          client.println();

         //stop de la connection tcp
          client.stop();
          delay(5000);
           //redémarage de l'esp
          ESP.restart();
        }
        else if( root == "/favicon.ico" )
        {
          client.println("HTTP/1.1 404 not disponible");
        }

      }
    }

    #ifdef DEBUG_MODE
      Serial.println(F("Client Disconnected."));
    #endif
    client.stop();


    }

}


String WifiGest::getRoot(String const & _data)
{
  int iStart = 0 , iend = 0;
  for(auto i = 0 ; i < _data.length() ; i++ )
  {
    if( _data[i]=='/' )
      iStart = i ;

    if( _data[i]==' ' )
    {
      iend = i ;
      break;
    }
  }


  return _data.substring(iStart , iend);
}

uint8_t const pin_relais_1 = 22;
uint8_t const pin_relais_2 = 23;
uint8_t const pin_relais_3 = 21;

uint8_t pin_A[4] = {36, 39,34 ,35};


uint8_t const pin_A5_36ref = 33;
uint8_t const pin_A6_24ref = 32;

SharedPtr<WifiGest> wifi_gest;
void setup() 
{

   //definition des sortie util esp a l'etat repos
  pinMode(pin_relais_1 , OUTPUT);
  digitalWrite( pin_relais_1 ,HIGH );
  
  pinMode( pin_relais_2, OUTPUT);
  digitalWrite( pin_relais_2 , HIGH );

    pinMode(pin_relais_3 , OUTPUT);
  digitalWrite( pin_relais_3 ,HIGH );

  #ifdef DEBUG_MODE
  Serial.begin(115200);
  #endif
  //création en memoire de l'instance de gestion des mode wifi
  wifi_gest = SharedPtr<WifiGest>( new WifiGest(AP_SSID , AP_PSWD , DNS ));

}

void loop() 
{
   //boolce principal surveillance serveur http
   wifi_gest->server_handle(pin_A , pin_A6_24ref , pin_A5_36ref ,pin_relais_1 ,pin_relais_2 , pin_relais_3);

   delay(100);
}
