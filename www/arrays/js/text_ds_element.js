function DataStructureList(enabledParent, disabledParent, onChange, enableDblClick) {
    this.dictionary = {};
    this.enParent = enabledParent;
    this.disParent = disabledParent;
    this.onChange = onChange;
    this.enableDblClick = enableDblClick;
}

DataStructureList.prototype.add = function(domElement) {
    this.dictionary[domElement.dataset.ds] = domElement;
    var self = this;
    domElement.ondblclick = function() { self.toggle(domElement.dataset.ds); };
};

DataStructureList.prototype.getEnabled = function() {
    var result = "";
    var enabledNodes = this.enParent.getElementsByClassName("qa-structure");
    for(var i = 0; i < enabledNodes.length; i++) {
        var dsName = enabledNodes[i].dataset.ds;
        if(this.enabled(dsName))
            result += dsName + '-';
    }
    return result.substr(0, Math.max(0, result.length - 1));
};

DataStructureList.prototype.setEnabled = function(dsList) {
    for(var key in this.dictionary)
        this.disable(key);
    var structs = dsList.split('-');
    for(var i = 0; i < structs.length; i++)
        this.enable(structs[i]);
};

DataStructureList.prototype.enabled = function(dsName) {
    return this.dictionary[dsName] 
        && this.dictionary[dsName].parentElement == this.enParent;
};

DataStructureList.prototype.isString = function(dsName) {
    return this.dictionary[dsName] 
        && this.dictionary[dsName].classList.contains('qa-structure-text');
};
DataStructureList.prototype.isFactorization = function(dsName) {
    return this.dictionary[dsName] 
        && this.dictionary[dsName].classList.contains('qa-structure-factorization');
};

DataStructureList.prototype.forEachEnabled = function(func) {
    var enabledNodes = this.enParent.getElementsByClassName("qa-structure");
    for(var i = 0; i < enabledNodes.length; i++) {
        var dsName = enabledNodes[i].dataset.ds;
        if(this.enabled(dsName)) func(dsName);
    }
};

DataStructureList.prototype.enable = function(dsName) {
    var el = this.dictionary[dsName];
    if(el) {
        this.enParent.appendChild(el);
        this.onChange();
    }
};

DataStructureList.prototype.disable = function(dsName) {
    var el = this.dictionary[dsName];
    if(el) {
        this.disParent.appendChild(el);
        this.onChange();
    }
};

DataStructureList.prototype.toggle = function(dsName) {
    if(this.enabled(dsName)) this.disable(dsName);
    else this.enable(dsName);
};
