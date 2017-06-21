<?php

/**
 *
 */
class IDER_Server
{

    /** Server Instance */
    public static $_instance = null;

    /** Options */
    public static $options = null;

    /** Default Settings */
    protected static $default_settings = array(
        'client_id' => '',
        'client_secret' => '',
        'extra_scopes' => '',
        'redirect_to_page' => true,
        'login_form_button' => true,
        'welcome_page' => 'my-account/ider-profile',
        'button_css' => '',
        'fields_mapping' => '
ider_sub=sub
first_name=given_name
last_name=family_name
# nickname=nickname
email=email
display_name=preferred_user_name
url=website
description=note

billing_address_1=address.street_address
billing_address_2=
billing_city=address.locality
billing_state=address.region
billing_postcode=address.postal_code
billing_country=address.country

# tmp
billing_first_name=given_name
billing_last_name=family_name
billing_company=
billing_phone=phone_number
billing_email=email
');


    function __construct()
    {
        self::init();
    }


    static function init()
    {
        spl_autoload_register(array(__CLASS__, 'autoloader'));

        //add_action("init", array(__CLASS__, "includes"));

        self::includes();

    }

    /**
     *  IDEROpenIDClient Initializer
     */
    public static function getIDerOpenIdClientIstance()
    {
        \IDERConnect\IDEROpenIDClient::$IDERLogFile = drupal_get_path('module', 'ider_login') . '/log/ider-connect.log';

        if (is_null(\IDERConnect\IDEROpenIDClient::$_instance)) {
            \IDERConnect\IDEROpenIDClient::$_instance = new \IDERConnect\IDEROpenIDClient(variable_get('ider_login_client_id'), variable_get('ider_login_client_secret'), variable_get('ider_login_scope_name'));
        }

        return \IDERConnect\IDEROpenIDClient::$_instance;
    }


    public static function IDerOpenIdClientHandler()
    {

        $queryV = drupal_get_query_parameters();


        try {

            $iderconnect = IDER_Server::getIDerOpenIdClientIstance();

            if (!empty($queryV['scope'])) {
                $iderconnect->setScope($queryV['scope']);
            }

            $iderconnect->authenticate();

            die(var_dump($iderconnect->requestUserInfo()));

            drupal_set_message(t("Hello $userInfo, the login logics are still missing :3"), 'error');

            IDER_Callback::handler($userInfo);

            exit;

        } catch (Exception $e) {
            //die("Session ID: " . session_id() . "<br />");

            die(var_dump($e->getMessage()));
            //IDER_Callback::access_denied($e->getMessage());
        } finally {
            exit;
        }
    }


    /**
     * populate the instance if the plugin for extendability
     * @return object plugin instance
     */
    public static function instance()
    {
        if (is_null(self::$_instance)) {
            self::$_instance = new self();
        }

        return self::$_instance;
    }

    /**
     * plugin includes called during load of plugin
     * @return void
     */
    public static function includes()
    {
        //  This should be used only when composer autoload fails to include classes
        //  self::loadPackage(IDER_PLUGIN_DIR.'vendor/phpseclib/phpseclib');
        //  self::loadPackage(IDER_PLUGIN_DIR.'vendor/jlmsrl/ider-openid-client-php');

    }

    private static function autoloader($class)
    {
        $path = drupal_get_path('module', 'ider_login');
        $paths = array();
        $exts = array('.php', '.class.php');

        $paths[] = $path;
        $paths[] = $path . 'includes/';

        foreach ($paths as $p)
            foreach ($exts as $ext) {
                if (file_exists($p . $class . $ext)) {
                    require_once($p . $class . $ext);
                    return true;
                }
            }

        return false;
    }

    private static function loadPackage($dir)
    {
        $composer = json_decode(file_get_contents("$dir/composer.json"), 1);
        $namespaces = $composer['autoload']['psr-4'];

        // Foreach namespace specified in the composer, load the given classes
        foreach ($namespaces as $namespace => $classpath) {
            spl_autoload_register(function ($classname) use ($namespace, $classpath, $dir) {
                // Check if the namespace matches the class we are looking for
                if (preg_match("#^" . preg_quote($namespace) . "#", $classname)) {
                    // Remove the namespace from the file path since it's psr4
                    $classname = str_replace($namespace, "", $classname);
                    $filename = preg_replace("#\\\\#", "/", $classname) . ".php";
                    include_once $dir . "/" . $classpath . "/$filename";
                }
            });
        }
    }

}

