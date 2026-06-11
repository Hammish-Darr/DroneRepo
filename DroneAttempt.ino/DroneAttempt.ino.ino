#include <esp_camera.h>
#include <WiFi.h>
#include <lwip/sockets.h>
#include <sys/types.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <strings.h>
#include <stdlib.h>

#define PWDN_GPIO_NUM     -1  // No power down pin on WROVER
#define RESET_GPIO_NUM    -1  // No reset pin
#define XCLK_GPIO_NUM     21
#define SIOD_GPIO_NUM     26  // SDA
#define SIOC_GPIO_NUM     27  // SCL
#define Y9_GPIO_NUM       35
#define Y8_GPIO_NUM       34
#define Y7_GPIO_NUM       39
#define Y6_GPIO_NUM       36
#define Y5_GPIO_NUM       19
#define Y4_GPIO_NUM       18
#define Y3_GPIO_NUM        5
#define Y2_GPIO_NUM        4
#define VSYNC_GPIO_NUM    25
#define HREF_GPIO_NUM     23
#define PCLK_GPIO_NUM     22

void startCamera() {
    camera_config_t config;
    config.ledc_channel = LEDC_CHANNEL_0;
    config.ledc_timer   = LEDC_TIMER_0;
    config.pin_d0       = Y2_GPIO_NUM;
    config.pin_d1       = Y3_GPIO_NUM;
    config.pin_d2       = Y4_GPIO_NUM;
    config.pin_d3       = Y5_GPIO_NUM;
    config.pin_d4       = Y6_GPIO_NUM;
    config.pin_d5       = Y7_GPIO_NUM;
    config.pin_d6       = Y8_GPIO_NUM;
    config.pin_d7       = Y9_GPIO_NUM;
    config.pin_xclk     = XCLK_GPIO_NUM;
    config.pin_pclk     = PCLK_GPIO_NUM;
    config.pin_vsync    = VSYNC_GPIO_NUM;
    config.pin_href     = HREF_GPIO_NUM;
    config.pin_sscb_sda = SIOD_GPIO_NUM;
    config.pin_sscb_scl = SIOC_GPIO_NUM;
    config.pin_pwdn     = PWDN_GPIO_NUM;
    config.pin_reset    = RESET_GPIO_NUM;
    config.xclk_freq_hz = 20000000;
    config.pixel_format = PIXFORMAT_JPEG;

    // WROVER has PSRAM, so we can use larger frames
    if (psramFound()) {
        config.frame_size   = FRAMESIZE_QVGA; // 
        config.jpeg_quality = 20;
        config.fb_count     = 4;              // double buffer for smoother capture
    } else {
        config.frame_size   = FRAMESIZE_QVGA; // fallback
        config.jpeg_quality = 15;
        config.fb_count     = 2;
    }

    esp_err_t err = esp_camera_init(&config);
    if (err != ESP_OK) {
        Serial.printf("Camera init failed: 0x%x\n", err);
        return;
    }

    Serial.println("Camera init OK");
}

void connectBasicWifi(const char * ssid, const char * password){
  //WiFi.begin(ssid, password);
  Serial.println();
  Serial.println("******************************************************");
  Serial.print("Connecting to ");
  Serial.println(ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
      delay(500);
        Serial.print(".");
  }
  WiFi.setHostname("ESP32_Camera_Module");
  Serial.println("");
  Serial.println("WiFi connected");
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
}

int sockfd, charn, newsockfd, portno;
long unsigned int clilen;
char IPAddr[16];

struct sockaddr_in servaddr, cliaddr;
struct hostent * serv = NULL;
bool isOff = false;


void setup() {



  Serial.begin(9600);
  // put your setup code here, to run once:
  connectBasicWifi("NETGEAR99", "magicalearth146");

  startCamera();

  portno = 8080;
  
  sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if(sockfd < 0){
    Serial.println("Socket creation error occurs");
  }
  
  if(1 == 2){//This is my way of commenting out cause FUCK you
    while(serv == NULL){
      Serial.println("Input server IP address:");
      while(Serial.available() == 0){
        delay(100);
      }
      if(Serial.available() > 0){
        if(Serial.available() > sizeof(IPAddr)-1){
          Serial.println("Invalid IP Address input");
          Serial.readStringUntil('\n');
        }
        else{
          IPAddr[Serial.available()] = '\0';
          strcpy(IPAddr, Serial.readStringUntil('\n').c_str());
        }
      }
      Serial.println(IPAddr);
      serv = gethostbyname(IPAddr);
      if(serv == NULL){
        Serial.println("Error resolving server by hostname");
      }

    }
  }

  bzero((char *) &servaddr.sin_zero, sizeof(servaddr.sin_zero));
  servaddr.sin_family = AF_INET;
  //bcopy((char *)serv->h_addr, (char *)&servaddr.sin_addr.s_addr, serv->h_length); //Copy from found server address to sock serv addr
  servaddr.sin_port = htons(portno);
  servaddr.sin_addr.s_addr = INADDR_ANY;

  if(bind(sockfd, (struct sockaddr * ) &servaddr, sizeof(servaddr)) > 0){ //??SHould this be greater than??
    Serial.println("Socket binding error occurs");
  }
  listen(sockfd, 5);

  newsockfd = accept(sockfd, (struct sockaddr * ) &cliaddr, &clilen);



  //if (connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) < 0){
  //  printf("Server connection error\n");
  //}



}

void loop() {

  if((Serial.available() > 0 && Serial.readStringUntil('\n') == "quit") || isOff){
    isOff = true;
    
  }
  else{

    camera_fb_t *fb = esp_camera_fb_get(); //Gets buffer info to new image
    if (!fb) {
        Serial.println("Capture failed");
        return;
    }
    uint32_t len = htonl(fb->len);

    write(newsockfd, &len, sizeof(len));

    charn = write(newsockfd, fb->buf, fb->len);
    Serial.printf("Frame: %d bytes, %dx%d\n", fb->len, fb->width, fb->height);
    //Serial.write(fb->buf, fb->len);

    esp_camera_fb_return(fb);
  }
}
