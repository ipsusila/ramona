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
	"bytes"
	"encoding/binary"
	"database/sql"
	MQTT "git.eclipse.org/gitroot/paho/org.eclipse.paho.mqtt.golang.git"
	_ "github.com/lib/pq"
)

/*
	MQTT packet sent by Arduino.
*/
type RadwatchPacket struct {
	Len			byte
	Token		uint16
	ClientId	uint32
	Hour		uint32
	Minute		uint16
	Second		float32
	Count		uint16
	Cps			float32
	SecuSv		float32
	ErrSec		float32
	Cpm			float32
	MinuSv		float32
	ErrMin		float32
	Pir			float32
	Temp		float32
	Humidity	float32	
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
	MQ_TOPIC	= "bsensors/#"
	MQ_THISID	= "mq2dbbin"
	
	DB_USER     = "monlink"
    DB_PASSWORD = "monLink2015"
    DB_NAME     = "monlink"
)

/*
 * Sensor handler, receive data in JSON then insert it to DB.
*/
var fSensorHandler MQTT.MessageHandler = func(client *MQTT.Client, msg MQTT.Message) {	
	//convert to JSON
	var pkt RadwatchPacket
	buf := bytes.NewBuffer(msg.Payload());
	err := binary.Read(buf, binary.LittleEndian, &pkt);
	if err == nil {
		go saveradwatchPacket(&pkt)
	} else {
		fmt.Printf("Error converting Radwatch Packet %+v\n", err)
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
	nsecs := float32(pkt.Hour) * 60 * 60 + float32(pkt.Minute) * 60 + pkt.Second
	vals := []float32{nsecs, float32(pkt.Hour), float32(pkt.Minute), pkt.Second, float32(pkt.Count), 
		pkt.Cps, pkt.SecuSv, pkt.ErrSec, pkt.Cpm, pkt.MinuSv, pkt.ErrMin,
		pkt.Pir, pkt.Temp, pkt.Humidity};
	
	for i := 0; i < len(vals); i++ {
		_, err := stmt.Exec(serid+uint32(i), ts, vals[i])
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
