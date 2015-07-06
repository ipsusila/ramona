/*
	Subscribe to sensors/{id}, parsed received data then insert into DB
	Author  : I Putu Susila
	Date    : 22.06.2015
	
	Require :
	  PAHO MQTT Clinet 
*/

package main

import (
	"os"
	"os/signal"
	"syscall"
	"fmt"
	"time"
	"encoding/json"
	"database/sql"
	MQTT "git.eclipse.org/gitroot/paho/org.eclipse.paho.mqtt.golang.git"
	_ "github.com/lib/pq"
)

/*
	MQTT packet sent by Arduino.
*/
type RadwatchPacket struct {
	ClientId	int		`json:"cid"`
	Hour		float64	`json:"hh"`
	Minute		float64	`json:"mm"`
	Second		float64	`json:"ss"`
	Count		float64	`json:"cnt"`
	Cps			float64	`json:"cps"`
	SecuSv		float64	`json:"uss"`
	ErrSec		float64	`json:"es"`
	Cpm			float64	`json:"cpm"`
	MinuSv		float64	`json:"usm"`
	ErrMin		float64	`json:"em"`
	Pir			float64	`json:"pir"`
	Temp		float64	`json:"temp"`
	Humidity	float64	`json:"hum"`	
}

/*
	SQL:

	CREATE TABLE measurement1 (
		serid				INTEGER NOT NULL PRIMARY KEY,
		measurement_ts		TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
		measurement_value	DOUBLE	
	);
*/

const (
	MQ_BROKER	= "tcp://192.168.1.10:1883"
	MQ_TOPIC	= "sensors/#"
	MQ_THISID	= "mq2db"
	
	DB_USER     = "monlink"
    DB_PASSWORD = "monLink2015"
    DB_NAME     = "monlink"
)

/*
 * Sensor handler, receive data in JSON then insert it to DB.
*/
var fSensorHandler MQTT.MessageHandler = func(client *MQTT.Client, msg MQTT.Message) {	
	//convert to JSON
	pkt := new(RadwatchPacket)
	err := json.Unmarshal(msg.Payload(), pkt) 
	if err == nil {
		go saveradwatchPacket(pkt)
	} else {
		fmt.Printf("Error converting JSON %+v\n", err)
	}
}

func setupDatabase() {
	//do nothing
}

/*
	Save packet to database (async run)
*/
func saveradwatchPacket(pkt *RadwatchPacket) {
	fmt.Printf("Save: %+v\n", pkt)
	if pkt.ClientId <= 0 {
		return
	}

	dbinfo := fmt.Sprintf("user=%s password=%s dbname=%s sslmode=disable",
    	DB_USER, DB_PASSWORD, DB_NAME)
    db, err := sql.Open("postgres", dbinfo)
    checkErr(err)
    defer db.Close()

	var sStmt string = "insert into measurement1(serid, measurement_ts, measurement_value) values ($1, $2, $3)"
	stmt, err := db.Prepare(sStmt)
	checkErr(err)
	defer stmt.Close()

	ts := time.Now()
	serid := pkt.ClientId;
	nsecs := pkt.Hour * 60 * 60 + pkt.Minute * 60 + pkt.Second
	vals := []float64{nsecs, pkt.Hour, pkt.Minute, pkt.Second, pkt.Count, 
		pkt.Cps, pkt.SecuSv, pkt.ErrSec, pkt.Cpm, pkt.MinuSv, pkt.ErrMin,
		pkt.Pir, pkt.Temp, pkt.Humidity};
	
	for i := 0; i < len(vals); i++ {
		_, err := stmt.Exec(serid+i, ts, vals[i])
		checkErr(err)
	}
}

/*
	Check error
*/
func checkErr(err error) {
    if err != nil {
        panic(err)
    }
}

func main() {
	//setup MQTT options for client
	opts := MQTT.NewClientOptions().AddBroker(MQ_BROKER)
	opts.SetClientID(MQ_THISID)
	opts.SetDefaultPublishHandler(fSensorHandler)

	//create and start a client using the above ClientOptions
	c := MQTT.NewClient(opts)
	if token := c.Connect(); token.Wait() && token.Error() != nil {
		panic(token.Error())
	}

	//subscribe to the topic /sensors/# (whole topic under sensors)
	if token := c.Subscribe(MQ_TOPIC, 0, nil); token.Wait() && token.Error() != nil {
		fmt.Println(token.Error())
		os.Exit(1)
	}
	
	//wait signaled
	chSig := make(chan os.Signal)
	signal.Notify(chSig, syscall.SIGINT, syscall.SIGTERM)
	fmt.Println("Signal: ", <-chSig)

	//unsubscribe from MQTT
	if token := c.Unsubscribe(MQ_TOPIC); token.Wait() && token.Error() != nil {
		fmt.Println(token.Error())
		os.Exit(1)
	}

	//wait 250 msec before disconnect
	c.Disconnect(250)
}
