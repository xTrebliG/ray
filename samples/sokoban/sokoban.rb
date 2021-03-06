$:.unshift File.expand_path(File.dirname(__FILE__) + "/../../lib")
$:.unshift File.expand_path(File.dirname(__FILE__) + "/../../ext")

require 'ray'

module Sokoban
  class InvalidLevel < StandardError; end

  class Level
    include Ray::Helper
    include Enumerable

    def self.open(filename)
      level = File.open(filename) { |io| new(io) }

      if block_given?
        yield level
      else
        level
      end
    end

    def initialize(io_or_string)
      @solved = false
      @moves = []

      @content = io_or_string.is_a?(String) ? io_or_string : io_or_string.read
      parse_content
    end

    def [](x, y)
      return :empty if x < 0 || y < 0

      line = @objects[y]
      return :empty unless line

      line[x] || :empty
    end

    def []=(x, y, value)
      @objects[y][x] = value
      raise_event :tile_changed, self, x, y, value if raiser_runner
    end

    attr_reader :character_pos

    def can_move?(direction)
      obj = next_obj = nil
      obj, next_obj = next_objects(direction)

      return true if obj == :empty || obj == :storage
      return false if obj == :wall

      # obj is a crate
      return next_obj == :empty || next_obj == :storage
    end

    def move(direction)
      make_move(direction)
    end

    def each(&block)
      return to_enum unless block_given?

      @objects.each do |ary|
        ary.each(&block)
      end
    end

    def each_with_pos
      return to_enum(:each_with_pos) unless block_given?

      @objects.each_with_index do |ary, y|
        ary.each_with_index do |obj, x|
          yield obj, x, y
        end
      end
    end

    def reset
      @moves.clear
      parse_content
    end

    def undo
      return if @moves.empty?

      move = @moves.last
      opposite_dir = case move[:direction]
                     when :left  then :right
                     when :right then :left
                     when :up    then :down
                     when :down  then :up
                     end

      x, y = character_pos
      old_pos, = next_positions(move[:direction])
      crate, = next_objects(move[:direction])

      replacement = (self[x, y] == :man_on_storage) ? :crate_on_storage : :crate

      make_move(opposite_dir, false, false)

      if move[:moved_crate]
        if crate == :crate
          self[*old_pos] = :empty
          self[x, y] = replacement
        else
          self[*old_pos] = :storage
          self[x, y] = replacement
        end
      end

      @moves.delete_at(-1)
      check_solved
    end

    def score
      @moves.size
    end

    def solved?
      @solved
    end

    private
    def char_to_object(char)
      case char
      when "@" then :man
      when "o", "$" then :crate
      when "#" then :wall
      when "." then :storage
      when "*" then :crate_on_storage
      when "+" then :man_on_storage
      when " " then :empty
      else
        raise InvalidLevel, "'#{char}' isn't a valid level character"
      end
    end

    def find_character_pos
      @objects.each_with_index do |ary, y|
        ary.each_with_index do |obj, x|
          return [x, y] if obj == :man || obj == :man_on_storage
        end
      end
    end

    def parse_content
      @objects = []

      @content.each_line.with_index do |line, y|
        @objects << []

        line.chomp.each_char.with_index do |char, x|
          @objects.last << char_to_object(char)
        end
      end

      each_with_pos do |obj, x, y|
        raise_event :tile_changed, self, x, y, obj if raiser_runner
      end

      @character_pos = find_character_pos
      self
    end

    def check_solved
      if !include? :crate
        @solved = true
        raise_event(:level_solved, self) if raiser_runner
      else
        @solved = false
      end
    end

    def next_positions(direction)
      x, y = character_pos

      case direction
      when :left
        (1..2).map { |i| [x - i, y] }
      when :right
        (1..2).map { |i| [x + i, y] }
      when :up
        (1..2).map { |i| [x, y - i] }
      when :down
        (1..2).map { |i| [x, y + i] }
      end
    end

    def next_objects(direction)
      next_positions(direction).map { |(x, y)| self[x, y] }
    end

    def make_move(direction, check_for_solve = true, count_move = true)
      x, y = character_pos
      obj, next_obj = next_objects(direction)
      first_pos, sec_pos = next_positions(direction)

      on_storage = (self[x, y] == :man_on_storage)
      replacement = on_storage ? :storage : :empty

      @moves << {
        :direction => direction,
        :moved_crate => obj == :crate || obj == :crate_on_storage
      } if count_move

      if obj == :empty
        self[*first_pos] = :man
        self[x, y]       = replacement

        @character_pos = first_pos
      elsif obj == :storage
        self[*first_pos] = :man_on_storage
        self[x, y]       = replacement

        @character_pos = first_pos
      elsif obj == :crate
        if next_obj == :empty
          self[*sec_pos]   = :crate
          self[*first_pos] = :man
          self[x, y]       = replacement

          @character_pos = first_pos
        elsif next_obj == :storage
          self[*sec_pos]   = :crate_on_storage
          self[*first_pos] = :man
          self[x, y]       = replacement

          @character_pos = first_pos
        end
      elsif obj == :crate_on_storage
        @moves[-1][:moved_crate] = true

        if next_obj == :empty
          self[*sec_pos]   = :crate
          self[*first_pos] = :man_on_storage
          self[x, y]       = replacement

          @character_pos = first_pos
        elsif next_obj == :storage
          self[*sec_pos]   = :crate_on_storage
          self[*first_pos] = :man_on_storage
          self[x, y]       = replacement

          @character_pos = first_pos
        end
      end

      check_solved if check_for_solve
    end
  end

  class LevelScene < Ray::Scene
    scene_name :sokoban_level

    TileWidth = 32

    def setup(filename)
      @filename = filename
      @level = Level.open(filename)

      @tiles = []
      @level.each_with_pos do |obj, x, y|
        obj = Ray::Polygon.rectangle([0, 0, TileWidth, TileWidth], color_for(obj),
                                     1, Ray::Color.black)
        obj.pos = [TileWidth * x, TileWidth * y]

        (@tiles[y] ||= [])[x] = obj
      end
    end

    def register
      @level.event_runner = event_runner

      [:left, :right, :up, :down].each do |dir|
        on :key_press, key(dir) do
          next if @level.solved?

          if @level.can_move?(dir)
            @level.move(dir)
          end
        end
      end

      on :key_press, key(:r) do
        next if @level.solved?
        @level.reset
      end

      on :key_press, key(:u) do
        next if @level.solved?
        @level.undo
      end

      on :level_solved do
        puts "Level solved!"
        pop_scene
      end

      on :tile_changed do |level, x, y, value|
        color = color_for(value)
        @tiles[y][x].each { |point| point.color = color }
      end
    end

    def render(win)
      @tiles.each do |line|
        line.each do |tile|
          win.draw tile
        end
      end
    end

    def color_for(obj)
      case obj
      when :man              then Ray::Color.white
      when :crate            then Ray::Color.gray
      when :wall             then Ray::Color.new(91, 59, 17)
      when :storage          then Ray::Color.yellow
      when :crate_on_storage then Ray::Color.green
      when :man_on_storage   then Ray::Color.red
      when :empty            then Ray::Color.black
      end
    end
  end

  class Game < Ray::Game
    def initialize
      super("Sokoban")

      LevelScene.bind(self)
      push_scene(:sokoban_level, File.expand_path(File.join(File.dirname(__FILE__), "level_1")))
    end

    def register
      add_hook :quit, method(:exit!)
    end
  end
end

Sokoban::Game.new.run
