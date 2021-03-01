<?php
    /* 
    IMPORTANT - PLEASE READ ME
        This is the ONLY file that I will use to validate your solution's implementation. Please keep in mind that only the changes done to this file
        will be tested, and if you modify anything in any other files those changes won't be taken in account when I validate your solution.
        Also, please do not rename the file.

        In a separate file (named answers.txt) answer the following questions for each function you implement:
            * What vulnerabilities can there be in that function 
                (take in account the fact that the function may not be vulnerable and explicitly say so if you consider it to be that way)
            * What specific mitigation you used for each of the vulnerabilities listed above
        
        For the function named 'get_language_php' which is already implemented make sure to answer and do all the steps required that are listed
        above the implementation.

    DELIVERY REQUIREMENTS
        When delivering your solution, please ensure that you create a .zip archive file (make sure it's zip, not 7z, rar, winzip, etc)
        with the name "LastnameFirstname.zip" (for example MunteaAndrei.zip or RatiuRazvan.zip) and in the root of the zip file please 
        add the student_delivery.php file modified by you (keep the name as it is) and answers.txt file where you answered the questions.
    */
    
    
    $_SESSION["message_timer"] = null;
    function _prevent_message_spam()
    {
        if(time() - $_SESSION["message_timer"] <= 5 && $_SESSION["message_timer"] != null)
        {
            return false;
        }
        else
        {
            $_SESSION["message_timer"] = time();
            return true;
        }
    }

    $_SESSION["login_timer"] = null;
    $_SESSION["login_tries"] = 0;

    function _prevent_login_bf()
    {
        if($_SESSION["login_tries"] >= 2)
        {
            if($_SESSION["login_timer"] == null)
            {
                $_SESSION["login_timer"] = time();
            }
            else if(time() - $_SESSION["login_timer"] > 10)
            {
                $_SESSION["login_tries"] = 0;
                $_SESSION["login_timer"] = null;
                return true;
            }
            return false;
        }
        return true;
    }


	function _validate_username($username)
	{
		
		if(preg_match('/[^a-zA-Z0-9_\-\.]/', $username))
			return false;
		return true;
    }
    
    function _validate_string_for_xss($string)
	{
		if(preg_match('/[\\<>\"\'\`]/', $string))
			return false;
		return true;
    }
    
	function _validate_path($path)
	{
		if(preg_match('/[\*\:\\\?]/', $path))
            return false;
        if(preg_match('/\.\./', $path))
			return false;
		return true;
    }

    function _validate_user_is_loged_in()
    {
        if(! isset($_SESSION['cookie']))
            return false;
        return true;
    }
    
    function _validate_user_is_who_it_says_it_is($uname)
    {
        if($_SESSION['cookie'] != $uname)
            return false;
        return true;

    }

    /* Implement query_db_login - this function is used in login.php */
    /* 
        Description - Must query the database to obtain the username that matches the 
        input parameters ($username, $password), or must return null if there is no match.
        The password is stored as MD5, so the query must convert the password received as parameter to
        MD5 and AFTER that interogate the DB with the MD5.
        PARAMETERS:
            $username: username field from post request
            $password: password field from post request
        MUST RETURN:
            null - if user credentials are not correct
            username - if credentials match a user
    */

    function query_db_login($username, $password) 
    {
        if(!_prevent_login_bf())
        {
            echo "Bruteforce attack detected. Timer set for 10 sec. <br>";
            return null;
        }
        if(_validate_user_is_loged_in())
        {
            echo "An user is already logged in. <br>";
            return null;
        }

		if(_validate_username($username) == false)
		{
            echo "Username is invalid. SQL injection detected <br>";
            
            if($_SESSION["login_tries"] <= 2)
                $_SESSION["login_tries"] += 1;
            
            return null;
		}
		
        $conn = get_mysqli();
        $found = null;
		
		$cryptedpasswd = md5($password);
		
		$query = "SELECT * FROM users WHERE username = '$username' AND password = '$cryptedpasswd'";
		
		if($result = $conn->query($query)) 
		{
            if($result->num_rows)
            {
                $found = $username;
            }
		}			
		else
		{
			echo "Database connection error!";
        }
        
        if($found == null)
        {
            if($_SESSION["login_tries"] <= 2)
                $_SESSION["login_tries"] += 1;
        }
        else
        {
            $_SESSION["login_tries"] = 0;
        }

        $conn->close();
        return $found;
    }

    /* Implement get_message_rows - this function is used in index.php */
    /* 
        Function must query the db and fetch all the entries from the 'messages' table
        (username, message - see MUST RETURN for more details) and return them in a separate array, 
        or return an empty array if there are no entries.
        PARAMETERS:
            No parameters
        MUST RETURN:
            array() - containing each of the rows returned by mysqli if there is at least one message
                      (code will use both $results['username'] and $results['message'] to display the data)
            empty array() - if there are NO messages
    */
    function get_message_rows() 
    {
        if(!_validate_user_is_loged_in())
        {
            echo "You must be logged in to perform this action! <br>";
            return false;
        }

        $conn = get_mysqli();
        $results = array();
        
		$query = "SELECT * FROM messages";

        $result = $conn->query($query)->fetch_all(MYSQLI_BOTH);
        foreach ($result as $row)
        {
            array_push($results, $row);
        }
        
        $conn->close();
        return $results;
    }
    
    /* Implement add_message_for_user - this function is used in index.php */
    /* 
        Function must add the message received as parameter to the database's 'message' table.
        PARAMETERS:
            $username - username for the user submitting the message
            $message - message that the user wants to submit
        MUST RETURN:
            Return is irrelevant here
    */
    function add_message_for_user($username, $message) 
    {
        if(!_validate_user_is_loged_in())
        {
            echo "You must be logged in to perform this action! <br>";
            return false;
        }
        
        if(!_prevent_message_spam())
        {
            echo "Less than 5 seconds between messagess! Spam detected <br>";
            return false;
        }

        $conn = get_mysqli();
        $results = array();

        if(strlen($message) == 0)
        {
            echo "Void message! <br>";
            return;
        }
        
        if(_validate_string_for_xss($message) == false)
        {
            echo "SQL/XSS injection detected <br>";
			return;
        }

        if(_validate_username($username) == false)
        {
            echo "Username is invalid. SQL injection detected <br>";
			return;
        }

        if(!_validate_user_is_who_it_says_it_is($username))
        {
            echo "You must be logged in to perform this action! <br>";
            return false;
        }

        $query = "INSERT INTO messages (username, message) VALUES ('$username', '$message')";

        if($conn->query($query) !== true)
            echo "Sql connection error";
        
        
        $conn->close();
    }

    /* Implement is_valid_image - this function is used in index.php */
    /* 
        This function will validate if the file contained at $image_path is indeed an image.
        PARAMETERS:
            $image_path: path towards the file on disk
        MUST RETURN:
            true - file is an image
            false - file is not an image
    */
    function is_valid_image($image_path) 
    {
        
        $finfo = new finfo();
        $filedesc = $finfo->file($image_path);

        if($filedesc === false)
            return false;

        $isimage = (strstr($filedesc, "image data") !== false);

        return $isimage;

    }

    /* Implement add_photo_to_user - this function is used in index.php */
    /* 
        This function must update the 'users' table and set the 'file_userphoto' field with 
        the value given to the $file_userphoto parameter
        PARAMETERS:
            $username - user for which to update the row
            $file_userphoto - value to be put in the 'file_userphoto' column (a path to an image)
        MUST RETURN:
            Return is irrelevant here
    */
    function add_photo_path_to_user($username, $file_userphoto) 
    {
        if(!_validate_user_is_loged_in())
        {
            echo "You must be logged in to perform this action! <br>";
            return false;
        }

        if(_validate_username($username) == false)
		{
			echo "Username is invalid. SQL injection detected <br>";
			return;
        }        
        
        if(is_valid_image($file_userphoto) == false)
        {
            echo "Invalid photo path <br>";
			return;
        }

        if(!_validate_user_is_who_it_says_it_is($username))
        {
            echo "You must be logged in to perform this action! <br>";
            return;
        }

        $conn = get_mysqli();
        $query = "UPDATE users SET file_userphoto = '$file_userphoto' WHERE username = '$username'";
        if($conn->query($query) === true) {
            echo "User photo added succesfuly<br>";
        }
        else {
            echo "Sql error";
        }
        
        $conn->close();
        return;
    }

    /* Implement get_photo_path_for_user - this function is used in index.php */
    /* 
        This function must obtain from the 'users' table the field named file_userphoto and
        return is as a string. If there is nothing in the database, then return null.
        PARAMETERS:
            $username - user for which to query the file_userphoto column
        MUST RETURN:
            string - string containing the value from the DB, if there is such a value
            null - if there is no value in the DB
    */
    function get_photo_path_for_user($username) 
    {
        if(!_validate_user_is_loged_in())
        {
            echo "You must be logged in to perform this action! <br>";
            return null;
        }

        if(!_validate_username($username))
        {
            echo "Invalid username! <br>";
            return null;
        }

        $conn = get_mysqli();
        $path = null;
        $query = "SELECT file_userphoto FROM users WHERE username = '$username'";
        $row = $conn->query($query)->fetch_all(MYSQLI_BOTH);
        if($row === null)
        {
            echo "No photo!<br>";
            return null;
        }

        $path = $row[0][0];
        if(is_valid_image($path) == false)
        {
            return null;
        }
        $conn->close();
        return $path;
    }

    /* Implement get_memo_content_for_user - this function is used in index.php */
    /* 
        This function must open the memo file for the current user from it's folder and return its content as a string.
        If the memo does not exist, the function must return the string "No such file!".
        PARAMETERS:
            $username - user for which obtain the memo file
            $memoname - the name of the memo the user requested to see
        MUST RETURN:
            string containing the data from the memo file (it's content)
            "No such file!" if there's no such file.
    */
    function get_memo_content_for_user($username, $memoname) 
    {
        if(!_validate_user_is_loged_in())
        {
            echo "You must be logged in to perform this action! <br>";
            return "No such file!";
        }

        if(_validate_username($username) == false)
        {
            echo "Username is invalid. SQL injection detected <br>";
			return "No such file!";
        }

        if(_validate_path($memoname) == false)
        {
            echo "File path contains special characters that are not allowed! <br>";
			return "No such file!";
        }

        if(_validate_path($username) == false)
        {
            echo "File path contains special characters that are not allowed! <br>";
			return "No such file!";
        }

        if(!_validate_user_is_who_it_says_it_is($username))
        {
            echo "You must be logged in to perform this action! <br>";
            return "No such file!";
        }
        $path = getcwd()."/users/$username/$memoname";

        $myfile = fopen($path, "r") or die("No such file!");
        $contents = fread($myfile,filesize($path));
        fclose($myfile);

        return $contents;

    }

    /* 
        Evaluate the impact of 'get_language_php' by explaining what are the risks of this function's default implementation
        (the one you received) by answering the following questions:
            - What is the vulnerability present in this function?
            The input is not validated nor sanitized, making it vulnerable to a metacharacter exploit. I.e. "http://localhost/index.php?language=../users/cristi/oh_no"
            - What other vulnerability can be chained with this vulnerability to inflict damage on the web application and where is it present?
            The above-mentioned vulnerability could be chained with the ability of the user to upload any type of files (as memos) without the system performing a check if they contain code instead of just memos.
            (I.e. the hacker could upload "oh_no.php" as a memo)
            - What can the attacker do once he chains the two vulnerabilities?
            These two vulnerabilities chained together grant a hacker remote code execution privilleges on the server. 
            (I.e. when the "http://localhost/index.php?language=../users/cristi/oh_no" path is accessed, the attacker's "oh_no.php" file is executed and *https://www.youtube.com/watch?v=6TUeUL7EW9M&ab_channel=davstern1500* ) 
            - As a result an atacker could leak any information on the system (lack of Confidentiality of the system)
            - As a result an atacker could rewrite generic user's data alongside with any other data on the system (lack of Integrity of the system)
            - As a result an atacker could brick the system permanently (lack of Accesability of the system)
        After that, modify the get_language_php function to no longer present a security risk.
        This function is used in index.php
    */
    /*
        This function must return the path to the language file corresponding to the desired language or null if the file
        does not exist. All language files must be in the language folder or else they are not supported.
        PARAMETERS:
            $language - desired language (e.g en)
        MUST RETURN:
            path to the en language file (languages/en.php)
            null if the language is not supported
    */
    function get_language_php($language)
    {
        if(!_validate_user_is_loged_in())
        {
            echo "You must be logged in to perform this action! <br>";
            return null;
        }

        if(_validate_path($language) == false)
        {
            echo "Path contains metacharacters, invalid path, defaulting to english!";
			return null;
        }
        $language_path = "language/" . $language . ".php";
        if (is_file($language_path))
        {
            return $language_path;
        }
        return null;
    }
?>