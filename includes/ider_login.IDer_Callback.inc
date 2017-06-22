<?php

/**
 *
 */

class IDER_Callback
{

    static function handler($user_info)
    {

        $user_info = IDER_UserInfoManager::normalize($user_info);
        // TODO callback function
        IDER_Callback::defaultHandler($user_info);

    }


    // register or authenticate user
    static function defaultHandler($user_info)
    {

        // check if user exists by email
        // ps: if user uses same email on a new IDer profile the sub will be updated on the old profile
        //$user = get_user_by('email', $user_info->email);

        $user = user_load_by_mail($user_info->email);


        // check if user exists by sub
        if (!$user) {
            $user = user_load(
                array(
                    'ider_sub' => $user_info->sub
                )
            );
            // TODO: leverage future endpoint to check which side changed the email: local->no access and error msg, remote->update email
        }

        // if new, register first
        if (!$user) {

            $user_id = self::_do_register($user_info);

            $user = user_load(
                array(
                    'uid' => $user_id
                )
            );
        }

        // Log the User In
        self::_login($user);

        // update user data
        self::_update_ider_sub($user->uid, $user_info->sub);

        if (user_is_logged_in()) {
            // pass the controll to user defined functions and landing pages
            //apply_filters('after_callback_handler', $user_info, $_SESSION['openid_connect_scope']);

            drupal_goto(url(variable_get('ider_login_welcome_page')));
            exit;
        }

        IDER_Callback::access_denied("User unable to login.");
    }


    static function access_denied($errormsg, $mainmsg = null)
    {
        if (is_null($mainmsg)) {
            $mainmsg = "Error authenticating user";
        }

        wp_enqueue_style('ider-css', IDER_PLUGIN_URL . 'assets/css/general.css', false, IDER_CLIENT_VERSION, 'all');

        $error_msg = sanitize_text_field($errormsg);
        get_header();
        echo '<div class="container">';
        echo '<div class="row">';
        echo '<div class="col-md-12 col-sm-18">';
        echo '<header class="page-header">';
        echo '<h1 class="page-title">' . $mainmsg . '</h1>';
        echo '</header>';
        echo '<div class="errordiv">';
        echo '<p>Please try later.</p>';
        echo '<small>' . ucwords(str_replace('_', ' ', $error_msg)) . ' </small>';
        echo '</div>';
        echo '</div>';
        echo '</div>';
        echo '</div>';
        get_footer();
        die();

    }


    private static function _update_ider_sub($user_id, $userSub)
    {
        $account = user_load($user_id);
        $edit = array('ider_sub' => $userSub);
        user_save($account, $edit);

        // TMP: filling missing fields
        /*
        update_user_meta($user_id, 'billing_address_1', 'Via Roma 10');
        update_user_meta($user_id, 'billing_postcode', '10100');
        update_user_meta($user_id, 'billing_city', 'Torino');
        update_user_meta($user_id, 'billing_state', 'TO');
        */

    }


    private static function _do_register($user_info)
    {

        // Does not have an account. Register and then log the user in
        $random_password =  user_password(12);

        $new_user = array(
            'ider_sub' => $user_info->sub,
            'name' => $user_info->first_name . ' ' . $user_info->family_name,
            'mail' => $user_info->email,
            'pass' => $random_password,
            'status' => 1,
            'access' => REQUEST_TIME,
            'roles' => array(), // No other roles than Authenticated
        );

        $user = user_save(NULL, $new_user);
        $userId = $user->uid;
        user_logout();

        return $userId;
    }


    private static function _login($userToLogIn)
    {

        // User ID 1 is not allowed
        if ('1' === $userToLogIn->uid) {
            drupal_exit('For security reasons, admin cannot login via IDer.');
        }

        if (!user_is_logged_in()) {
            user_cookie_delete('name');
            user_cookie_delete('mail');
            user_cookie_delete('homepage');

            $formState = array('uid' => $userToLogIn->uid);
            user_login_submit(array(), $formState);

        }

    }


}