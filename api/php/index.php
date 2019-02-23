<?php

if ($_SERVER['REQUEST_METHOD'] === 'GET') {
	// User is requesting shoe IDs
	$shoeIds = getShoeIds();
	returnJsonResponse(200, $shoeIds);
} elseif ($_SERVER['REQUEST_METHOD'] === 'POST') {
	// User is issuing command to shoe

	// Check that the appropriate parameters are present
	if (!isset($_POST['shoeIds']) || !isset($_POST['color'])) {
		returnJsonResponse(400, ['success' => false, 'message' => 'color and shoeIds are required parameters']);
		return;
	}

	// Validate shoe IDs
	$shoeIds = $_POST['shoeIds'];
	if (!is_array($shoeIds)) {
		returnJsonResponse(400, ['success' => false, 'message' => 'shoeIds must be an array']);
		return;
	}

	if (count(array_intersect($shoeIds, getShoeIds())) !== count($shoeIds)) {
		returnJsonResponse(400, ['success' => false, 'message' => 'one or more invalid shoeIds']);
		return;
	}

	// Validate color and get RGB values
	$color = $_POST['color'];
	$colors = [
		'red' => [255, 0, 0],
		'green' => [0, 255, 0],
		'blue' => [0, 0, 255],
		'teal' => [0, 255, 255],
		'purple' => [255, 0, 255],
		'white' => [255, 255, 192],
		'orange' => [255, 16, 0],
		'yellow' => [255, 64, 0]
		'pink' => [255, 0, 32],
		'none' => [0, 0, 0],
		'off' => [0, 0, 0]
	];
	if (!in_array($color, array_keys($colors))) {
		returnJsonResponse(400, ['success' => false, 'message' => 'color must be one of: red, green, blue, teal, purple, white, orange, pink, none, off']);
		return;
	}
	$rgb = $colors[$color];

	// Tell each shoe what color to be by putting RGB values in its checkin file
	$shoeStatus = [];
	foreach ($shoeIds as $shoeId) {
		echo "processing $shoeId...\n";
		$filename = './checkin/' . $shoeId;
		$fileContents = implode(',', $rgb);
		$bytesWritten = file_put_contents($filename, $fileContents);
		$shoeStatus[$shoeId] = $bytesWritten ? 'color updated' : 'color update failed';
	}

	returnJsonResponse(202, $shoeStatus);
}

function getShoeIds(): array
{
	$shoeIds = [];
	foreach (new DirectoryIterator('./checkin') as $file) {
		if ($file->isDot()) {
			continue;
		}
		$shoeIds[] = $file->getFilename();
	}

	return $shoeIds;
}

function returnJsonResponse(int $statusCode, array $responseData)
{
	header('Content-type: application/json');
	http_response_code($statusCode);
	echo json_encode($responseData) . "\n";
}

