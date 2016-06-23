var colors = ["steelblue", "tomato", "orange", "darkseagreen", "slateblue", "rosybrown"];

//convert stats to data
var groups = [];
var data = [];

var convert = function(x, memOff) {
    var ds = {
        title:   x.title,
        tStart:  x.timeStart - stats.timeStart,
        tEnd:    x.timeEnd - stats.timeStart,
        memOff:  memOff + x.memOff,
        memPeak: memOff + x.memOff + x.memPeak,
        stats:   x.stats
    };

    if(x != stats && x.sub.length > 0) {
        groups.push(ds);
    } else if(x.sub.length == 0) {
        ds.color = colors[data.length % colors.length];
        data.push(ds);
    }

    for(var i = 0; i < x.sub.length; i++) {
        convert(x.sub[i], memOff + x.memOff);
    }
};

convert(stats, stats.memOff);

// Define dimensions
var margin = {top: 50, right: 200, bottom: 50, left: 100};
var width = 1280 - margin.left - margin.right;
var height = 720 - margin.top - margin.bottom;

// Define scales
var tDuration = stats.timeEnd - stats.timeStart;

var tUnit = "s";
var tScale = d3.scale.linear()
            .range([0, tDuration / 1000])
            .domain([0, tDuration]);

var memUnits = ["bytes", "KiB", "MiB", "GiB"];

var memUnit = 0;
var memDiv = 1;
{
    var u = 0;
    var memPeak = stats.memPeak;
    while(u < memUnits.length && memPeak > 1024) {
        memPeak /= 1024.0;
        memDiv *= 1024;
        u++;
    }

    memUnit = memUnits[u];
}

var memScale = d3.scale.linear()
                .range([0, stats.memPeak / memDiv])
                .domain([0, stats.memPeak]);

// Define axes
var x = d3.scale.linear()
            .range([0, width])
            .domain(tScale.range());

var y = d3.scale.linear()
            .range([height, 0])
            .domain(memScale.range());

var xAxis = d3.svg.axis().scale(x).orient("bottom").ticks(15);
var yAxis = d3.svg.axis().scale(y).orient("left");

// Create chart
var chart = d3.select("#chart svg")
    .attr("width", width + margin.left + margin.right)
    .attr("height", height + margin.top + margin.bottom)
    .append("g")
    .attr("transform", "translate(" + margin.left + "," + margin.top + ")");

// Draw bars
var bar = chart.selectAll("g.bar")
    .data(data).enter()
    .append("g")
    .attr("class", "bar")
    .attr("transform", function(d) {
        return "translate(" + x(tScale(d.tStart)) + ",0)";
     });

bar.append("rect")
    .attr("class", "mem")
    .attr("x", "0")
    .attr("y", function(d) { return y(memScale(d.memPeak)); })
    .attr("height", function(d) { return height - y(memScale(d.memPeak)); })
    .attr("width", function(d) { return x(tScale(d.tEnd - d.tStart)) })
    .attr("fill", function(d) { return d.color; });

bar.append("line")
    .attr("x1", "0")
    .attr("x2", function(d) { return x(tScale(d.tEnd - d.tStart)); })
    .attr("y1", function(d) { return y(memScale(d.memOff)); })
    .attr("y2", function(d) { return y(memScale(d.memOff)); });

/*
bar.append("text")
    .attr("x", function(d) { return x(tScale(d.tEnd - d.tStart)); })
    .attr("y", function(d) { return y(memScale(d.memPeak)); })
    .attr("dx", "-0.5em")
    .attr("dy", "1.5em")
    .text(function(d) { return d.title; });
*/

// Draw legend
var legend = chart.selectAll("g.legend")
            .data(data).enter()
            .append("g")
            .attr("class", "legend")
            .attr("transform", function(d) {
                return "translate(" + x.range()[1] + ",0)";
            });

legend.append("rect")
    .attr("fill", function(d) { return d.color; })
    .attr("x", "0")
    .attr("y", function(d, i) { return (i * 1.5) + "em" })
    .attr("width", "1em")
    .attr("height", "1em");

legend.append("text")
    .attr("x", "1.25em")
    .attr("y", function(d, i) { return (i * 1.5) + "em" })
    .attr("dy", "1em")
    .text(function(d) { return d.title; });

// Draw groups
var group = chart.selectAll("g.group")
            .data(groups).enter()
            .append("g")
            .attr("class", "group")
            .attr("transform", function(d) {
                return "translate(" + x(tScale(d.tStart)) + ",0)";
            });

group.append("text")
    .attr("x", function(d) { return x(tScale((d.tEnd - d.tStart) / 2)); })
    .attr("y", function(d) { return y(memScale(d.memPeak)); })
    .attr("dy", "-25")
    .text(function(d) { return d.title; });

group.append("line")
    .attr("x1", "0")
    .attr("x2", "0")
    .attr("y1", function(d) { return y(memScale(d.memPeak)); })
    .attr("y2", height);

group.append("line")
    .attr("x1", function(d) { return x(tScale(d.tEnd - d.tStart)); })
    .attr("x2", function(d) { return x(tScale(d.tEnd - d.tStart)); })
    .attr("y1", function(d) { return y(memScale(d.memPeak)); })
    .attr("y2", height);

group.append("line")
    .attr("x1", "0")
    .attr("x2", function(d) { return x(tScale(d.tEnd - d.tStart)); })
    .attr("y1", function(d) { return y(memScale(d.memPeak)); })
    .attr("y2", function(d) { return y(memScale(d.memPeak)); });

group.append("line")
    .attr("x1", function(d) { return x(tScale((d.tEnd - d.tStart) / 2)); })
    .attr("x2", function(d) { return x(tScale((d.tEnd - d.tStart) / 2)); })
    .attr("y1", "-20")
    .attr("y2", function(d) { return y(memScale(d.memPeak)); });

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
    .attr("cy", "0")
    .attr("r", "3");

// Draw axes
chart.append("g")
    .attr("class", "axis")
    .attr("transform", "translate(0," + height + ")")
    .call(xAxis);

chart.append("text")
    .attr("class", "axis-label")
    .attr("transform", "translate(" + width/2 + "," + height + ")")
    .attr("dy", "3em")
    .text("Time / " + tUnit);

chart.append("g")
    .attr("class", "axis")
    .call(yAxis);

chart.append("text")
    .attr("class", "axis-label")
    .attr("transform", "translate(0," + height/2 + ") rotate(-90)")
    .attr("dy", "-3em")
    .text("Memory Peak / " + memUnit);

// Utilities
var setMarker = function(xpos) {
    var t = tScale.invert(x.invert(xpos));

    for(var i = 0; i < data.length; i++) {
        var d = data[i];
        if(t >= d.tStart && t <= d.tEnd) {
            var memY = y(memScale(d.memPeak));

            marker.attr("transform", "translate(" + xpos + ",0)");
            marker.select("circle").attr("cy", memY);

            return {
                data: d,
                y:    memY
            };
        }
    }

    return null;
};

// Formatting functions
var formatTime = function(ms) {
    return tScale(ms).toFixed(3) + " " + tUnit;
};

var formatMem = function(mem) {
    return memScale(mem).toFixed(2) + " " + memUnit;
}

var formatPercent = function(pct) {
    return (pct * 100.0).toFixed(2) + " %";
}

// Events
var cachetStart = -1;
chart.on("mousemove", function() {
    var m = d3.mouse(this);
    var mx = m[0];
    if(mx > 0) {
        var marker = setMarker(mx);
        if(marker && marker.data) {
            var d = marker.data;
            if(d.tStart != cachetStart) {
                cachetStart = d.tStart;

                var dur = d.tEnd - d.tStart;
                var durPct = dur / tDuration;

                var tip = d3.select("#tip");

                tip.select(".title").text(d.title);
                tip.select(".duration").text(
                    formatTime(dur) + " (" + formatPercent(durPct) + ")");
                tip.select(".mempeak").text(formatMem(d.memPeak));
                tip.select(".memadd").text(formatMem(d.memPeak - d.memOff));

                var ext = tip.select("tbody.ext").html("");
                if(d.stats) {
                    var tr = ext.selectAll("tr").data(Object.keys(d.stats)).enter().append("tr");

                    tr.append("th").text(function(key) { return key + ":"; });
                    tr.append("td").text(function(key) { return d.stats[key]; });
                }
            }
        }

        var mdoc = d3.mouse(document.documentElement);
        d3.select("#tip")
            .style("display", "inline")
            .style("left", mdoc[0] + "px")
            .style("top", mdoc[1] + "px");
    }
});

