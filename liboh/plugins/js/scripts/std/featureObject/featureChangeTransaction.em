

std.featureObject.FeatureChangeTransaction = function (pres,commitNum)
{
    this.pres = pres;
    this.commitNum = commitNum;
    this.actions = [];
    this.haveCommitted = false;
};


std.featureObject.Action = function(type, index, data)
{
    this.type  = type;
    this.index = index;
    this.data  = data;
};


std.featureObject.Action.ADD    = 0;
std.featureObject.Action.REMOVE = 1;
std.featureObject.Action.CHANGE = 2;



std.featureObject.FeatureChangeTransaction.prototype.addField = function(index,data)
{
    var actionToTake =
        new std.featureObject.Action(std.featureObject.Action.ADD,index,data);
    this.actions.push(actionToTake);
};

std.featureObject.FeatureChangeTransaction.prototype.changeField = function(index,data)
{
    var actionToTake =
        new std.featureObject.Action(std.featureObject.Action.CHANGE,index,data);
    this.actions.push(actionToTake);
};


std.featureObject.FeatureChangeTransaction.prototype.removeField = function(index)
{
    var actionToTake =
        new std.featureObject.Action(std.featureObject.Action.REMOVE,index);
    this.actions.push(actionToTake);
};


std.featureObject.FeatureChangeTransaction.prototype.commit = function()
{
    if (this.haveCommited)
        throw new Error ('Error when attempting to commit ' +
                         'feature change transaction: have' +
                         ' already committed these actions.');
    this.haveCommitted = true;

    std.featureObject.commitFeatureChanges(this);
};

