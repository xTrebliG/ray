$:.unshift File.expand_path(File.dirname(__FILE__) + "/../../lib")
$:.unshift File.expand_path(File.dirname(__FILE__) + "/../../ext")

require 'ray'

Ray.game "Hello world!" do
  register { add_hook :quit, method(:exit!) }

  scene :rotation do
    @text = text "Hello world!", :at => [100, 100], :size => 30
    animations << color_variation(:from => Ray::Color.red, :to => Ray::Color.blue,
                                  :duration => 1).start(@text)

    render { |win| win.draw @text }
  end

  scenes << :rotation
end
