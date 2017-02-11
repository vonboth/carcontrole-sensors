
#define FUEL_SENDER A0

int fuelSender = 0;
int level = 0;
int mod = 0;
int full = 0;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

void loop() {
  // put your main code here, to run repeatedly:

  fuelSender = analogRead(FUEL_SENDER);
  level = map(fuelSender, 200, 600, 14, 0);

  String a = "";

  for (int i=0; i<=level; i++) {
    a += "#";
  }

  String pre = "F:" + a;
  
  
  Serial.println(pre);
}
