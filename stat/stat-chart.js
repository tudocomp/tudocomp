var colors = ["steelblue", "tomato", "orange", "darkseagreen", "slateblue", "rosybrown"];

//convert stats to data
var data = [];
var t0 = stats.timeStart;
var convert = function(x, memOff) {
    if(x.sub.length == 0) {
        data.push({
            title:   x.title,
            tStart:  x.timeStart - t0,
            tEnd:    x.timeEnd - t0,
            memOff:  memOff + x.memOff,
            memPeak: memOff + x.memOff + x.memPeak,
        });
    } else {
        for(var i  = 0; i < x.sub.length; i++) {
            convert(x.sub[i], memOff + x.memOff);
        }
    }
};
convert(stats, stats.memOff);

// Define dimensions
var margin = {top: 20, right: 100, bottom: 30, left: 80};
var width = 1280 - margin.left - margin.right;
var height = 720 - margin.top - margin.bottom;

// Define axes
var x = d3.scale.linear().range([0, width], .5);
var y = d3.scale.linear().range([height, 0]);

x.domain([0, d3.max(data, function(d) { return d.tEnd; })]);
y.domain([0, d3.max(data, function(d) { return d.memPeak; })]);

var xAxis = d3.svg.axis().scale(x).orient("bottom");
var yAxis = d3.svg.axis().scale(y).orient("left");

// Create chart
var chart = d3.select("#chart svg")
    .attr("width", width + margin.left + margin.right)
    .attr("height", height + margin.top + margin.bottom)
    .append("g")
    .attr("transform", "translate(" + margin.left + "," + margin.top + ")");

// Draw bars
var bar = chart.selectAll("g")
    .data(data).enter()
    .append("g")
    .attr("class", "bar")
    .attr("title", function(d) { return d.title; })
    .attr("transform", function(d) { return "translate(" + x(d.tStart) + ",0)"; });

bar.append("rect")
    .attr("class", "mem")
    .attr("x", "0")
    .attr("y", function(d) { return y(d.memPeak); })
    .attr("height", function(d) { return height - y(d.memPeak); })
    .attr("width", function(d) { return x(d.tEnd - d.tStart) })
    .attr("fill", function(d, i) { return colors[i % colors.length]; });

bar.append("line")
    .attr("x1", "0")
    .attr("x2", function(d) { return x(d.tEnd - d.tStart); })
    .attr("y1", function(d) { return y(d.memOff); })
    .attr("y2", function(d) { return y(d.memOff); });

bar.append("text")
    .attr("x", function(d) { return x(d.tEnd - d.tStart); })
    .attr("y", function(d) { return y(d.memPeak); })
    .attr("dx", "-0.5em")
    .attr("dy", "1.5em")
    .text(function(d) { return d.title; });

// Marker
var marker = chart.append("g")
    .attr("class", "marker")
    .attr("transform", "translate(0, 0)");

marker.append("line")
    .attr("x1", "0")
    .attr("x2", "0")
    .attr("y1", 0)
    .attr("y2", height);

marker.append("circle")
    .attr("cx", "0")
    .attr("cy", "100")
    .attr("r", "3");

// Draw axes
chart.append("g")
    .attr("class", "axis")
    .attr("transform", "translate(0," + height + ")")
    .call(xAxis);

chart.append("g")
    .attr("class", "axis")
    .call(yAxis);

// Utilities
var setMarker = function(xpos) {
    var t = x.invert(xpos);

    for(var i = 0; i < data.length; i++) {
        var d = data[i];
        if(t >= d.tStart && t <= d.tEnd) {
            marker.attr("transform", "translate(" + xpos + ",0)");
            marker.select("line").attr("y1", y(d.memPeak));
            marker.select("circle").attr("cy", y(d.memPeak));
            return d;
        }
    }

    return null;
};

setMarker(280);

// Events
chart
.on("mouseover", function() {
})
.on("mousemove", function() {
    var m = d3.mouse(this);
    var mx = m[0];
    if(mx > 0) {
        var d = setMarker(mx);
        if(d) {
            d3.select("#chart #info").html(d.title);
        }
    }
})
.on("mouseout", function() {
});

