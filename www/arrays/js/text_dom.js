var inField;
var outField;
var dataStructures;
var options;

var defaultStructures;
var defaultOptions;

var updateRequested = true;
var updateReady = true;
function updateHistory() {
    if(!updateReady) updateRequested = true;
    else {
        updateReady = false;
        updateHistoryInternal();
        setTimeout(function() {
            updateReady = true;
            if(updateRequested) {
                updateRequested = false;
                updateHistory();
            }
        }, 1000);
    }
}

function updateHistoryInternal() {
    var structsStr = dataStructures.getEnabled();
    var optsStr = options.getEnabled();
 
    var newQuery = $.query.empty();
    if(inField.value) newQuery = newQuery.set("text", inField.value);
    if(structsStr != defaultStructures) newQuery = newQuery.set("structures", structsStr);
    if(optsStr != defaultOptions) newQuery = newQuery.set("options", optsStr);
    
    window.history.replaceState("", "", window.location.pathname + newQuery.toString());
}

function updateTextAreas() {
    updateTextArea(inField);
    updateTextArea(outField);
}

function updateTextArea(area) {
    area.style.height = ""; 
    area.style.height = (10 + area.scrollHeight) + 'px';
}

function encodeWhitespaces(string) {
    return string.replace(/\n/g, '\u21b5').replace(/\s/g, '\u23b5');
}

function decodeWhitespaces(string) {
    return string.replace(/\u21b5/g, '\n').replace(/\u23b5/g, ' ');
}

var wasWhitespace = false;
function updateWhitespaces() {
    if(options.enabled("whitespace")) {
        var selStart = inField.selectionStart;
        var selEnd = inField.selectionEnd;
        inField.value = encodeWhitespaces(inField.value);
        inField.selectionStart = selStart;
        inField.selectionEnd = selEnd;
        wasWhitespace = true;
    }
    else if(wasWhitespace) {
        var selStart = inField.selectionStart;
        var selEnd = inField.selectionEnd;
        inField.value = decodeWhitespaces(inField.value);
        inField.selectionStart = selStart;
        inField.selectionEnd = selEnd;
        wasWhitespace = false;
    }
}

var varText, varIndex, varSA, varISA, varPHI, varLCP, varPLCP, varPSI, varF, varBWT, varLF;
function updateArrays() {
    
    updateWhitespaces();
    if(options.enabled("whitespace"))
        varText = decodeWhitespaces(inField.value);
    else 
        varText = inField.value;

    if(!varText) varText = inField.placeholder;

    var varBase = 0;
    if(options.enabled("baseone")) varBase = 1;
    if(options.enabled("dollar")) varText += '\0';

    if(varText.length > 0) {
        varIndex = indexArray(varText.length, varBase)
        varSA = suffixArray(varText, varBase);
        varISA = inverseSuffixArray(varSA, varBase);
        varPHI = phiArray(varSA, varISA, varBase);
        varLCP = lcpArray(varText, varSA, varBase);
        varPLCP = plcpArray(varISA, varLCP, varBase);
        varPSI = psiArray(varSA, varISA, varBase);
        varF = firstRow(varText, varSA, varBase);
        varBWT = bwt(varText, varSA, varBase);
        varLF = lfArray(varSA, varISA, varBase);
    }
    
    var sep = "";
    if(options.enabled("comma")) sep += ",";
    if(options.enabled("space")) sep += " ";
    
    var result = "";
    dataStructures.forEachEnabled(function(dsName) {
        var varDs = window["var" + dsName];
        if(dataStructures.isString(dsName)) {
            if(options.enabled("whitespace"))
                varDs = encodeWhitespaces(varDs);
            varDs = stringToString(varDs, sep, varBase);
        }
        else varDs = arrayToString(varDs, sep, varBase);
        result += padRight(dsName + ":", ' ', 7) + varDs + "\n";
    });
    outField.value = result.substr(0, result.length - 1);

    updateTextAreas();
    updateHistory();
}

function initDragAndDrop(listEnabled, listDisabled) {
    Sortable.create(listEnabled, {
        group: 'qa-structs',
        draggable: '.qa-structure',
        ghostClass: 'qa-structure-ghost',
        dragClass: 'qa-structure-drag',
        onSort: updateArrays
    });
    Sortable.create(listDisabled, {
        group: 'qa-structs',
        draggable: '.qa-structure',
        ghostClass: 'qa-structure-ghost',
        dragClass: 'qa-structure-drag'
    });
}

window.onload = function () {
    inField = document.getElementById('textSource');
    outField = document.getElementById('arraysDestination');
    structuresListEn = document.getElementById('qa-structures-enabled');
    structuresListDis = document.getElementById('qa-structures-disabled');
    
    // initalize data structure settings container
    var structureElements = document.getElementsByClassName("qa-structure");
    dataStructures = new DataStructureList(structuresListEn, structuresListDis, updateArrays, true);
    for(var i = 0; i < structureElements.length; i++) dataStructures.add(structureElements[i]);
    defaultStructures = dataStructures.getEnabled();
    
    // initialize option settings container
    var optionElements = document.getElementsByClassName("qa-option-cbx");
    options = new OptionList(updateArrays);
    for(var i = 0; i < optionElements.length; i++) options.add(optionElements[i]);
    defaultOptions = options.getEnabled();
    
    // parse configuration from GET url parameters
    var textquery = $.query.get("text").toString();
    if(textquery) inField.value = textquery;
    var queryStructures = $.query.get("structures").toString();
    if(queryStructures) dataStructures.setEnabled(queryStructures);
    var queryOptions = $.query.get("options").toString();
    if(queryOptions) options.setEnabled(queryOptions);    
    
    // update output while typing
    inField.oninput = updateArrays;
    inField.onpropertychange = updateArrays;

    updateArrays();
    updateHistoryInternal();
    initDragAndDrop(structuresListEn, structuresListDis);
};
