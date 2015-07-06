/*
	Subscribe to sensors/{id}, parsed received data then insert into DB
	Author  : I Putu Susila
	Date    : 22.06.2015

	Require :
	  PAHO MQTT Clinet
*/

package main

import (
	"fmt"
	"strconv"
	"os"
	"time"
	"database/sql"
	_ "github.com/lib/pq"
	"github.com/gonum/plot"
    	"github.com/gonum/plot/plotter"
    	"github.com/gonum/plot/plotutil"
    	"github.com/gonum/plot/vg"
)

/*
	SQL:

	CREATE TABLE measurement1 (
		serid				INTEGER NOT NULL PRIMARY KEY,
		measurement_ts		TIMESTAMP NOT NULL DEFAULT CURRENT_TIMESTAMP,
		measurement_value	DOUBLE
	);
*/

const (
	DB_USER     = "monlink"
    DB_PASSWORD = "monLink2015"
    DB_NAME     = "monlink"
)


func setupDatabase() {
	//do nothing
}

func getCount(serid uint32, from time.Time, to time.Time) uint32 {
	dbinfo := fmt.Sprintf("user=%s password=%s dbname=%s sslmode=disable",
        DB_USER, DB_PASSWORD, DB_NAME)
    db, err := sql.Open("postgres", dbinfo)
    checkErr(err)
    defer db.Close()

        var sStmt string = "select count(*) as cnt from measurement1 where serid = $1 and measurement_ts between $2 and $3"
        stmt, err := db.Prepare(sStmt)
        checkErr(err)
        defer stmt.Close()

        rows, err := stmt.Query(serid, from, to)
        checkErr(err)

	var cnt uint32 = 0
	if rows.Next() {
		err = rows.Scan(&cnt)
		checkErr(err)
	}
	return cnt
}

func drawChart(id int, serid uint32, from time.Time, to time.Time) {
	numRows := getCount(serid, from, to)

	//connect to database
	dbinfo := fmt.Sprintf("user=%s password=%s dbname=%s sslmode=disable",
    	DB_USER, DB_PASSWORD, DB_NAME)
    db, err := sql.Open("postgres", dbinfo)
    checkErr(err)
    defer db.Close()

	var sStmt string = "select measurement_ts, measurement_value from measurement1 " +
		"where serid = $1 and measurement_ts between $2 and $3 order by measurement_ts asc"
	stmt, err := db.Prepare(sStmt)
	checkErr(err)
	defer stmt.Close()

	rows, err := stmt.Query(serid, from, to)
	checkErr(err)

	//prepare plotter
	p, err := plot.New()
	checkErr(err)

	//file
	txtName := "chart/data_" + strconv.Itoa(int(serid)) + "_" + strconv.Itoa(id) + ".txt"
	f, err := os.Create(txtName)
    checkErr(err)
	defer f.Close()

	idx := 0
	pts := make(plotter.XYs, numRows)
	for rows.Next() {
		var ts time.Time
		var val float64

		err = rows.Scan(&ts, &val)
        checkErr(err)

		//draw graph
		sts := ts.Sub(from).Seconds()
		pts[idx].X = sts
		pts[idx].Y = val
		idx++

		//write file
		f.WriteString(strconv.FormatFloat(sts, 'f', 2, 32))
		f.WriteString(",")
		f.WriteString(strconv.FormatFloat(val, 'f', 6, 32))
		f.WriteString("\n")
	}
	f.Sync()

	err = plotutil.AddLinePoints(p, from.String(), pts)
	checkErr(err)
	fName := "chart/chart_" + strconv.Itoa(int(serid)) + "_" + strconv.Itoa(id) + ".png"
	err = p.Save(8*vg.Inch, 4*vg.Inch, fName)
	checkErr(err)

	fmt.Printf("Draw chart for %d between %+v and %+v (%d)\n", serid, from, to, numRows)
}

/*
	Save packet to database (async run)
*/
func drawCharts(serid uint32) {
	dbinfo := fmt.Sprintf("user=%s password=%s dbname=%s sslmode=disable",
    	DB_USER, DB_PASSWORD, DB_NAME)
    db, err := sql.Open("postgres", dbinfo)
    checkErr(err)
    defer db.Close()

	var sStmt string = "select measurement_ts from measurement1 where measurement_value < 0.25 and serid=1 order by measurement_ts asc"
	stmt, err := db.Prepare(sStmt)
	checkErr(err)
	defer stmt.Close()

	rows, err := stmt.Query()
	checkErr(err)

	var ts time.Time
	if rows.Next() {
		rows.Scan(&ts)
	}
	id := 0
	for rows.Next() {
		var from time.Time
		var to time.Time

		from = ts
		rows.Scan(&to)
		ts = to
		id++
		to = to.Add(-time.Duration(1)*time.Second)
		drawChart(id, serid, from, to)
	}
	drawChart(id, serid, ts, time.Now())
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
	var inpStr string
	for serid := 1; serid <= 14; serid++ {
		drawCharts(uint32(serid))
	}
	fmt.Scanf("%s", &inpStr)
}
