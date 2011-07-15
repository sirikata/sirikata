system.import('std/shim/quaternion.em');

var testVecs = [
    <1, 0, 0>,
    <0, 1, 0>,
    <0, 0, 1>,
    <1, 1, 0>,
    <1, 0, 1>,
    <0, 1, 1>,
    <1, 1, 1>
];
var NUM_RANDOM_TESTS = 10;

function getRandomVec() {
	return <Math.random() * 4 - 2, Math.random() * 4 - 2, 
			Math.random() * 4 - 2>;
}

function test(dir, up) {
	up = up || <0, 1, 0>;
	var q = util.Quaternion.fromLookAt(dir, up);
	var forward = q.mul(<0, 0, -1>);
    var newUp = q.mul(<0, 1, 0>);
	var reason = '';
	if((forward - dir.normal()).length() > 0.001) {
		reason += 'incorrect forward vector\n';
	}
	if(Math.abs(newUp.dot(up.normal()) -
			dir.normal().cross(up.normal()).length()) > 1e-04) {
		reason += 'incorrect up vector\n';
	}
	
	if(reason != '')
		system.print('lookAt(' + std.core.pretty(dir) + ', ' +
                std.core.pretty(up) + ') failed:\n   gave ' +
				std.core.pretty(q) + '\n   which produces\n   forward: ' +
                std.core.pretty(forward) + '\n   up: ' +
                std.core.pretty(newUp) + '\n');
		system.print(reason);
        return false;
    } else {
        return true;
    }
}

function runTests() {
    for(var i in testTypes) {
        var failures = testTypes[i]();

        if(failures) {
            system.print('' + failures + ' tests failed.\n');
            return;
        } else {
            system.print('all tests passed.\n');
        }
    }
}

function randomBasic() {
    var failures = 0;
    system.print('Running basic random vector tests...');
    for(var i = 0; i < NUM_RANDOM_TESTS; i++) {
        if(!test(getRandomVec().normal()))
            failures++;
    }
    return failures;
}

function edgeBasic() {
    var failures = 0;
    system.print('Running basic edge case tests...');
    for(var i in testVecs) {
        if(!test(testVecs[i].normal()))
            failures++;
        if(!test(testVecs[i].normal().neg()))
            failures++;
    }
    return failures;
}

function upBasic() {
    var failures = 0;
    system.print('Running basic up-vector variation tests...');
    for(var i in testVecs) {
        if(!test(getRandomVec().normal(), testVecs[i].normal()))
            failures++;
        if(!test(getRandomVec().normal(), testVecs[i].normal().neg()))
            failures++;
    }
    return failures;
}

function randomUp() {
    var failures = 0;
    system.print('Running random up-vector tests...');
    for(var i = 0; i < NUM_RANDOM_TESTS; i++) {
        if(!test(getRandomVec().normal(), getRandomVec().normal()))
            failures++;
    }
    return failures;
}

function denormalized() {
    var failures = 0;
    system.print('Running denormalized direction tests...');
    for(var i = 0; i < NUM_RANDOM_TESTS; i++)
    {
        if(!test(getRandomVec()))
            failures++;
    }
    return failures;
}

function denormEdge() {
    var failures = 0;
    system.print('Running denormalized edge case tests...');
    for(var i in testVecs) {
        if(!test(testVecs[i].scale(2)))
            failures++;
        if(!test(testVecs[i].scale(.2)))
            failures++;
        if(!test(testVecs[i].scale(-2)))
            failures++;
        if(!test(testVecs[i].scale(-.2)))
            failures++;
    }
    return failures;
}

function denormUp() {
    var failures = 0;
    system.print('Running denormalized up vector tests...');
    for(var i = 0; i < NUM_RANDOM_TESTS; i++) {
        if(!test(getRandomVec(), getRandomVec()))
            failures++;
    }
    return failures;
}

function denormUpEdge() {
    var failures = 0;
    system.print('Running denormalized up vector edge case tests...');
    for(var i in testVecs) {
        if(!test(getRandomVec(), testVecs[i].scale(2)))
            failures++;
        if(!test(getRandomVec(), testVecs[i].scale(.2)))
            failures++;
        if(!test(getRandomVec(), testVecs[i].scale(-2)))
            failures++;
        if(!test(getRandomVec(), testVecs[i].scale(-.2)))
            failures++;
    }
    return failures;
}

var testTypes = [
    randomBasic, edgeBasic, upBasic, randomUp,
    denormalized, denormEdge, denormUp, denormUpEdge
];
