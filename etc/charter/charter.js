/**
 * This is the tudocomp charter's core script.
 *
 * It generates the SVG for a given JSON dataset either for direct export
 * or for interactive display in the web application.
 */

// The available bar colors, which will be rotated
var colors = [
    "#4682B4", // steelblue
    "#FF6347", // tomato
    "#FF8C00", // darkorange
    "#8FBC8F", // darkseagreen
    "#6A5ACD", // slateblue
    "#BC8F8F", // rosybrown
    "#FF69B4", // hotpink
    "#DDA0DD", // plum
    "#90EE90", // lightgreen
    "#708090", // slategrey
    "#FFD700", // gold
    "#CD853F", // peru
    "#9ACD32", // yellowgreen
];

// The special "gray" color used for "disabled" bars
var grayColor = "#CCC";
var grayFontColor = "#888";

// Units for memory data points
var memUnits = ["bytes", "KiB", "MiB", "GiB"];

//Shade color by percentage.
//Source: http://stackoverflow.com/questions/5560248 (shadeColor2 by Pimp Trizkit)
function shadeColor(color, percent) {
    var f=parseInt(color.slice(1),16),t=percent<0?0:255,p=percent<0?percent*-1:percent,R=f>>16,G=f>>8&0x00FF,B=f&0x0000FF;
    return "#"+(0x1000000+(Math.round((t-R)*p)+R)*0x10000+(Math.round((t-G)*p)+G)*0x100+(Math.round((t-B)*p)+B)).toString(16).slice(1);
}

/**
 * Simple XML builder.
 * 
 * Each element is represented by a class of this type and has an attribute
 * object and an array of children.
 */
function XMLNode(name, attributes) {
    this.xmlName = name;
    this.attributes = attributes;
    this.content = "";
    this.children = [];

    // appends a child
    this.appendChild = function(elem) {
        this.children.push(elem);
        return elem;
    }

    this.toStringInternal = function(level) {
        var indent = " ".repeat(2 * level);

        var s = indent + "<" + this.xmlName;
        for(var key in this.attributes) {
            //if(!node.hasOwnProperty(key)) continue;
            s += " " + key + "=\"" + this.attributes[key] + "\"";
        }

        if(this.children.length == 0) {
            // no children
            s += ">" + this.content + "</" + this.xmlName + ">\n";
        } else {
            // one or more children
            s += ">\n";
            for(var i = 0; i < this.children.length; i++) {
                s += this.children[i].toStringInternal(level + 1);
            }
            s += indent + "</" + this.xmlName + ">\n";
        }
        return s;
    }

    // prints an XML string
    this.toString = function() {
        return this.toStringInternal(0);
    }
}

/**
 * Computes the maximum of an array using a mapping function.
 */
function max(arr, f) {
    var result = 0;
    for(var i = 0; i < arr.length; i++) {
        result = Math.max(result, f(arr[i]));
    }
    return result;
}

/**
 * Computes an aesthetic tick distance for the given range (starting from 0)
 * and the desired amount of ticks.
 */
function tickDistance(range, n, base) {
    var q = range / n;
    var lg = Math.log(range) / Math.log(base);

    // compute upper and lower bound
    var bu = Math.pow(base, Math.floor(lg));
    var bl = Math.pow(base, Math.floor(lg) - 1);

    // calculate distance based on closest bound
    var d;
    if(q / bu > 0.5) {
        d = bu * Math.round(q / bu);
    } else {
        d = bl * Math.round(q / bl);
    }

    // adjust if very close to lower bound
    if(Math.floor(range / d) <= n / 2) {
        d = Math.floor(d / 2);
    }
    
    return d;
}

/**
 * Manages the chart data.
 *
 * Unlike the input JSON dataset, it has time values aligned and memory peaks
 * cumulated.
 */
function ChartData(raw) {
    // Raw
    if(raw.meta) {
        this.raw = x.data;
        this.meta = x.meta;
    } else {
        this.raw = raw;
    }

    // Data
    this.data = [];
    this.groups = [];

    // converts a raw JSON dataset to displayable charter data
    this.convert = function(x, memOff, level) {
        var ds = {
            title:     x.title,
            tStart:    x.timeStart - this.raw.timeStart,
            tEnd:      x.timeEnd - this.raw.timeStart,
            tDuration: (x.timeEnd - x.timeStart),
            memOff:    memOff + x.memOff,
            memPeak:   memOff + x.memOff + x.memPeak,
            memFinal:  memOff + x.memOff + x.memFinal,
            stats:     x.stats,
            sub:       []
        };

        if(x != this.raw && x.sub.length > 0) {
            ds.level = level;
            this.groups.push(ds);
        } else if(x.sub.length == 0) {
            ds.color = colors[this.data.length % colors.length];
            this.data.push(ds);
        }

        for(var i = 0; i < x.sub.length; i++) {
            ds.sub.push(this.convert(x.sub[i], memOff + x.memOff, level + 1));
        }

        return ds;
    };

    // import raw JSON data
    this.root = this.convert(this.raw, this.raw.memOff, -1);
}

/**
 * Creates a chart based on the given JSON dataset, also containing the SVG.
 *
 * The following options can be passed via the options object:
 *
 * svgWidth:    the width of the SVG
 * svgHeight:   the width of the SVG
 * drawGroups:  toggle group brackets
 * drawOffsets: toggle memory offsets
 * drawLegend:  toggle legend
 */
function chart(input, options) {
    // create chart object
    var chartData = new ChartData(input);
    this.data = chartData;

    // Define dimensions
    var groupBracketHeight = 15;
    var groupLevelMax = max(chartData.groups, function(g) { return g.level; });
    var groupLevelIndent = function(level) {
        return (groupLevelMax - level) * 30;
    };

    var margin = {
        top: 10,
        left: 55,
        bottom: 50,
        right: 15
    };

    if(options.drawGroups && chartData.groups.length > 0) {
        margin.top += groupLevelIndent(-1) + groupBracketHeight;
    }

    if(options.drawLegend) {
        var longestTitle = max(
            chartData.data, function(d) { return d.title.length; });

        margin.right += longestTitle * 8;
    }

    this.width = options.svgWidth - margin.left - margin.right;
    this.height = options.svgHeight - margin.top - margin.bottom;

    // Determine time scale
    var tDuration = chartData.raw.timeEnd - chartData.raw.timeStart;
    var tUnit, tScale;
    if(tDuration > 1000) {
        tUnit = "s";
        tScale = 0.001;
    } else {
        tUnit = "ms";
        tScale = 1;
    }

    this.timeToPx = function(ms) {
        return (ms / tDuration) * this.width;
    };

    // Determine memory scale
    var memMaxPeak = chartData.raw.memPeak;
    var memUnit, memScale;
    {
        var memDiv = 1;
        var u = 0;
        var memPeak = chartData.raw.memPeak;
        while(u < memUnits.length && memPeak > 1024) {
            memPeak /= 1024.0;
            memDiv *= 1024;
            u++;
        }

        memUnit = memUnits[u];
        memScale = 1 / memDiv;
    }

    this.memToPx = function(bytes) {
        return (bytes / memMaxPeak) * this.height;
    };

    // generate SVG
    var svg = new XMLNode("svg", {
        xmlns:   "http://www.w3.org/2000/svg",
        version: "1.1",
        style:   "font-family: sans-serif; font-size: 10pt;",
        width:   options.svgWidth,
        height:  options.svgHeight,
    });
    
    var gZoom = svg.appendChild(new XMLNode("g", {
        "class":   "zoom",
        transform: "scale(1.0)"
    }));

    var gChart = gZoom.appendChild(new XMLNode("g", {
        transform: "translate(" + margin.left + "," + margin.top + ")"
    }));

    // visualize data
    for(var i = 0; i < chartData.data.length; i++) {
        var d = chartData.data[i];
        var shortPhase = (this.timeToPx(d.tDuration) < 1.0);

        // draw bar
        {
            var gBar = gChart.appendChild(new XMLNode("g", {
                "class": "bar",
                transform: "translate(" + this.timeToPx(d.tStart) + ",0)"
            }));

            // main bar
            gBar.appendChild(new XMLNode("rect", {
                "class": "mem",
                x:       0,
                y:       this.height - this.memToPx(d.memPeak),
                width:   this.timeToPx(d.tDuration),
                height:  this.memToPx(d.memPeak),
                fill:    d.color
            }));

            // offset bar
            if(options.drawOffsets && d.memOff > 0) {
                gBar.appendChild(new XMLNode("rect", {
                    "class": "mem",
                    x:       0,
                    y:       this.height - this.memToPx(d.memOff),
                    width:   this.timeToPx(d.tDuration),
                    height:  this.memToPx(d.memOff),
                    fill:    shadeColor(d.color, -0.25)
                }));
            }
        }

        // draw legend entry
        if(options.drawLegend) {
            // TODO: create parent node for legend
            var gLegendEntry = gChart.appendChild(new XMLNode("g", {
                "class": "legend",
                transform: "translate(" + (this.width + 5) + ",0)"
            }));

            var legendY = (i * 1.35) + "em";

            // color box
            gLegendEntry.appendChild(new XMLNode("rect", {
                x:       0,
                y:       legendY,
                width:   "1em",
                height:  "1em",
                fill:    (shortPhase ? grayColor : d.color)
            }));

            // text
            var tLegendText = gLegendEntry.appendChild(new XMLNode("text", {
                x:       "1.15em",
                y:       legendY,
                dy:      "0.85em"
            }));

            tLegendText.content = d.title;

            if(shortPhase) {
                tLegendText.attributes.style =
                    "fill: " + grayColor + ";" +
                    "font-style: italic; text-decoration: line-through;";
            }
        }
    }

    // draw groups
    if(options.drawGroups) {
        for(var i = 0; i < chartData.groups.length; i++) {
            var d = chartData.groups[i];
            
            var gGroup = gChart.appendChild(new XMLNode("g", {
                "class": "group",
                transform: "translate(" + this.timeToPx(d.tStart) + "," + 
                    (this.height - this.memToPx(d.memPeak) - groupLevelIndent(d.level)) + ")"
            }));

            // text
            gGroup.appendChild(new XMLNode("text", {
                x:     this.timeToPx(d.tDuration / 2),
                y:     -groupBracketHeight - 3,
                style: "font-weight: bold; text-anchor: middle"
            })).content = d.title;

            // curly brace
            var x1 = 0;
            var x2 = this.timeToPx(d.tDuration);

            var w = x2 - x1;
            var w4 = w/4;
            var h = groupBracketHeight;
            var h2 = h/2;
            var h4 = h/4;

            gGroup.appendChild(new XMLNode("path", {
                d: "M " + x1 + " 0"
                   + " q 0 " + (-h2) + ", " + w4 + " " + (-h2)
                   + " q " + w4 + " 0, " + w4 + " " + (-h2)
                   + " q 0 " + h2 + ", " + w4 + " " + h2
                   + " q " + w4 + " 0, " + w4 + " " + h2,
                style: "fill: none; stroke: black; stroke-width: 1.5"
            }));

            // dashed lines
            gGroup.appendChild(new XMLNode("line", {
                x1: 0,
                x2: 0,
                y1: 0,
                y2: this.height,
                style: "stroke: rgba(0,0,0,0.25); stroke-width: 1;" +
                       "stroke-dasharray: 5,2;"
            }));

            gGroup.appendChild(new XMLNode("line", {
                x1: this.timeToPx(d.tDuration),
                x2: this.timeToPx(d.tDuration),
                y1: 0,
                y2: this.height,
                style: "stroke: rgba(0,0,0,0.25); stroke-width: 1;" +
                       "stroke-dasharray: 5,2;"
            }));
        }
    }

    // draw X axis
    {   
        var gAxis = gChart.appendChild(new XMLNode("g", {
            "class":   "axis",
            transform: "translate(0," + this.height + ")"
        }));

        // main line
        gAxis.appendChild(new XMLNode("line", {
            x1: 0,
            x2: this.width,
            y1: 0,
            y2: 0,
            style: "stroke: black; stroke-width: 1;"
        }));

        // label
        gAxis.appendChild(new XMLNode("text", {
            "class":   "axis-label",
            transform: "translate(" + (this.width / 2) + ",0)",
            dy:        "3em",
            style:     "font-weight: bold; font-style: italic;" + 
                       "text-anchor: middle;"
        })).content = "Time / " + tUnit;

        // ticks
        var d = tickDistance(tDuration, 10, 10);
        for(var t = 0; t <= tDuration; t += d) {
            var gTick = gAxis.appendChild(new XMLNode("g", {
                "class":   "tick",
                transform: "translate(" + this.timeToPx(t) + ",0)"
            }));

            gTick.appendChild(new XMLNode("line", {
                x1:    0,
                x2:    0,
                y1:    0,
                y2:    5,
                style: "stroke: black;"
            }));

            gTick.appendChild(new XMLNode("text", {
                x:  0,
                y:  5,
                dy: "1em",
                style: "text-anchor: middle;"
            })).content = (tScale * t).toString();
        }
    }

    // draw Y axis
    {
        var gAxis = gChart.appendChild(new XMLNode("g", {
            "class": "axis"
        }));

        // main line
        gAxis.appendChild(new XMLNode("line", {
            x1: 0,
            x2: 0,
            y1: 0,
            y2: this.height,
            style: "stroke: black; stroke-width: 1;"
        }));

        // label
        gAxis.appendChild(new XMLNode("text", {
            "class":   "axis-label",
            transform: "translate(0," + (this.height / 2) + ") " +
                       "rotate(-90)",
            dy:        "-3em",
            style:     "font-weight: bold; font-style: italic;" + 
                       "text-anchor: middle;"
        })).content = "Memory / " + memUnit;

        // ticks
        var d = tickDistance(memMaxPeak, 8, 16);
        for(var mem = 0; mem <= memMaxPeak; mem += d) {
            var gTick = gAxis.appendChild(new XMLNode("g", {
                "class":   "tick",
                transform: "translate(0," + (this.height - this.memToPx(mem)) + ")"
            }));

            gTick.appendChild(new XMLNode("line", {
                x1:    -5,
                x2:    0,
                y1:    0,
                y2:    0,
                style: "stroke: black;"
            }));

            gTick.appendChild(new XMLNode("text", {
                x:  -8,
                y:  0,
                dy: "0.25em",
                style: "text-anchor: end;"
            })).content = (memScale * mem).toString();
        }
    }

    // generate XML
    this.svg = svg.toString();
}

// command-line interface to get the SVG as a string directly
function drawSVG(json, options_json) {
    // parse JSON
    var input   = JSON.parse(json);
    var options = JSON.parse(options_json);

    return new chart(input, options).svg;
}

